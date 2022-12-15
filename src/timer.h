/*
 * @Author: lxk
 * @Date: 2022-10-15 16:00:37
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-15 22:26:02
 */

#ifndef __SPADGER_TIMER_H__
#define __SPADGER_TIMER_H__

#include "thread.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace spadger {

class TimerManager;

// ================================================================
// ======================   Timer  ================================
// ================================================================

class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager; // Timer只能通过TimerManager创建

public:
  typedef std::shared_ptr<Timer> ptr;

  bool cancel();
  bool refresh();
  bool reset(uint64_t ms, bool from_now);

private:
  // 设置为private函数是因为只能在TimerManager中设置
  Timer(uint64_t ms, std::function<void()> cb, bool recurring,
        TimerManager *manager);
  Timer(uint64_t ms);
  bool m_recurring = false; // 是否循环执行
  uint64_t m_ms = 0;        // 多久执行一次
  uint64_t m_next = 0; // 下次执行的精确的时间 = (init_time + k * m_ms)
  std::function<void()> m_cb;
  TimerManager *m_manager = nullptr;

private:
  // 定时器的比较句柄
  struct Comparator {
    bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const;
  };
};

// ================================================================
// ======================   TimerManager  =========================
// ================================================================

class TimerManager {
  friend class Timer;

public:
  typedef RWMutex RWMutexType;

  TimerManager();
  virtual ~TimerManager();

  // 普通定时器
  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                      bool recurring = false);
  // 条件定时器，在满足条件的状态下才会执行
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                               std::weak_ptr<void> weak_cond,
                               bool recurring = false);

  uint64_t getNextTimer();
  void listExpiredCb(std::vector<std::function<void()>> &cbs);
  bool hasTimer();

protected:
  virtual void onTimerInsertedAtFront() = 0;
  void addTimer(Timer::ptr timer, RWMutexType::WriteLock &lock);

private:
  // 判断是否调了时间
  bool detectClockRollover(uint64_t now_ms);

private:
  RWMutexType m_mutex;
  std::set<Timer::ptr, Timer::Comparator> m_timers;
  bool m_tickled = false;
  uint64_t m_previousTime = 0;
};

} // namespace spadger

#endif