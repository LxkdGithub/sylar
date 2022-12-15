/*
 * @Author: lxk
 * @Date: 2022-10-15 16:10:48
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 10:54:15
 */
#include "timer.h"
#include "util.h"

namespace spadger {

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
  if (!lhs && !rhs) {
    return false;
  }
  if (!lhs) {
    return true;
  }
  if (!rhs) {
    return false;
  }
  // 先比较next时间，再比较地址
  if (lhs->m_next < rhs->m_next) {
    return true;
  } else if (lhs->m_next > rhs->m_next) {
    return false;
  } else {
    return lhs.get() < rhs.get();
  }
}

// ================================================================
// ======================   Timer  ===============================
// ================================================================

// ------------------------ Timer() --------------------------
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
  m_next = m_ms + getCurrentMS();
}

// 只是构造一个含有m_next的Timer,方便临时使用,比如比较查找
Timer::Timer(uint64_t next) : m_next(next) {}

// ------------------------ cancel ----------------------------
bool Timer::cancel() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (m_cb) {
    m_cb = nullptr;
    auto it = m_manager->m_timers.find(shared_from_this());
    m_manager->m_timers.erase(it);
    return true;
  }
  return false;
}

// ------------------------ reset -----------------------------
bool Timer::reset(uint64_t ms, bool from_now) {
  if (m_ms == ms && from_now == false) {
    return true; // does not need to change.
  }
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  auto it = m_manager->m_timers.find(shared_from_this());
  if (it == m_manager->m_timers.end()) {
    return false;
  }
  m_manager->m_timers.erase(it);
  uint64_t last_start = 0;
  if (from_now) {
    last_start = time(0);
  } else {
    last_start = m_next - m_ms;
  }
  m_ms = ms;
  m_next = last_start + m_ms;
  // addTimer中有检测是否当前事件出于最前面，如果是的话，需要唤醒查看是否到计时的时间了
  m_manager->addTimer(shared_from_this(), lock);
  return true;
}

// ------------------------ refresh -----------------------------
bool Timer::refresh() {
  // 以当前时刻为准设置m_next
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }
  auto it = m_manager->m_timers.find(shared_from_this());
  if (it == m_manager->m_timers.end()) {
    return false;
  }
  m_manager->m_timers.erase(it);
  m_next = getCurrentMS() + m_ms;
  // 直接插入，因为时间只会更加靠后面 不会向前
  m_manager->m_timers.insert(shared_from_this());
  return true;
}

// ================================================================
// ======================   TimerManager  =========================
// ================================================================

TimerManager::TimerManager() { m_previousTime = getCurrentMS(); }

TimerManager::~TimerManager() {}

// ------------------------ addTimer --------------------------
// 普通定时器
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
  Timer::ptr timer(new Timer(ms, cb, recurring, this));
  RWMutexType::WriteLock lock(m_mutex);
  addTimer(timer, lock);
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
  // lock()如果对象还在，返回shared_ptr，否则返回空指针
  std::shared_ptr<void> tmp = weak_cond.lock();
  if (tmp) {
    cb();
  }
}

// ------------------------ addConditionTimer --------------------------
// 条件定时器，在满足条件的状态下才会执行
// 其实就是在cb外再包装一层，见上文的OnTimer
Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recurring) {
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

// ------------------------ getNextTimer --------------------------
// 获取下一个最近到时的timer
uint64_t TimerManager::getNextTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  m_tickled = false; // 前面没有行为需要去唤醒了
  if (m_timers.empty()) {
    return ~0ull;
  }
  const Timer::ptr &next = *m_timers.begin();
  uint64_t now_ms = getCurrentMS();
  if (now_ms >= next->m_next) {
    return 0; // 如果已经错过第一个了，只能返回0了
  } else {
    return next->m_next - now_ms;
  }
}

// ------------------------ listExpiredCb --------------------------
// 取出所有的过时的timer的cb
void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
  uint64_t now_ms = getCurrentMS();
  {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_timers.empty()) {
      return;
    }
  }
  RWMutexType::WriteLock lock(m_mutex);
  if (m_timers.empty()) {
    return;
  }
  bool rollover = detectClockRollover(now_ms);
  // 没有回调时间并且第一个计时器到时也在之后，那就没有过期的计时器事件了
  if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
    return;
  }

  Timer::ptr now_timer(new Timer(now_ms));
  // lower_bound和upper_bound的效果是一样的 因为不会相等
  auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
  while (it != m_timers.end() && (*it)->m_next == now_ms) {
    ++it;
  }
  std::vector<Timer::ptr> expired;
  expired.insert(expired.begin(), m_timers.begin(), it);
  m_timers.erase(m_timers.begin(), it);

  cbs.reserve(expired.size());
  for (auto &timer : expired) {
    cbs.push_back(timer->m_cb);
    if (timer->m_recurring) {
      timer->m_next = now_ms + timer->m_ms;
      m_timers.insert(timer);
    } else {
      timer->m_cb = nullptr; // TODO: 有必要吗
    }
  }
}

// ------------------------ addTimer(timer, lock) --------------------------
void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock) {
  auto it = m_timers.insert(timer).first; // first是iter second是bool
  // 需要去唤醒了 如果m_tickled = true, 表明前面有插入需要唤醒但是
  // 还没有被唤醒(因为醒来之后肯定是要调用getNextTimer的，就会设置m_tickled
  // = false了)，也就是说信号只需要设置一次即可
  bool need_to_notify = false;
  if (it == m_timers.begin() && !m_tickled) {
    // 位于第一位并且前面没有唤醒行为，那就唤醒
    need_to_notify = true;
    m_tickled = true;
  }
  lock.unlock();

  if (need_to_notify) {
    onTimerInsertedAtFront();
  }
}

// ------------------------ hasTimer() --------------------------
bool TimerManager::hasTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  return !m_timers.empty();
}

// ------------------------ detectClockRollover --------------------------
bool TimerManager::detectClockRollover(uint64_t now_ms) {
  bool rollover = false;
  if (now_ms < m_previousTime && now_ms < m_previousTime - 60 * 60 * 1000) {
    rollover = true;
  }
  m_previousTime = now_ms;
  return rollover;
}

} // namespace spadger