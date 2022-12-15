/*
 * @Author: lxk
 * @Date: 2021-12-27 15:48:59
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-20 16:17:55
 */
#include "scheduler.h"
#include "hook.h"
#include "log.h"
#include "mutex.h"
#include "src/fiber.h"
#include "thread.h"
#include "util.h"
// 也可以将所有头文件放进spadger.h 直接导入

namespace spadger {

static spadger::Logger::ptr g_logger = SPADGER_LOG_NAME("system");

static thread_local Scheduler *t_scheduler = nullptr;
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
    : m_name(name) {
  SPADGER_ASSERT(threads > 0);

  if (use_caller) { // 是否使用当前线程
    spadger::Fiber::
        GetThis(); // 设置了当前线程的主协程(这里的协程是线程的主协程
                   // 每个线程都有的) 按道理来说
                   // 如果swapOut都和m_mainFiber切换的话是不是就不需要了
    --threads; // 当前线程算上的话 自然就可以少开辟一个线程了

    SPADGER_ASSERT(GetThis() == nullptr); // 确保当前没有调度器
    t_scheduler = this;

    // 设置当前调度器的主协程 执行run方法 因为当前线程也要使用嘛
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    spadger::Thread::SetName(m_name); // 将当前线程放进线程池

    t_scheduler_fiber = m_rootFiber.get();
    m_rootThread = spadger::GetThreadId(); // 设置当前线程为调度器的主线程
    m_threadIds.push_back(m_rootThread);
  } else {
    m_rootThread = -1; // 没有主线程
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  SPADGER_ASSERT(m_stopping);
  if (GetThis() == this) { // 除了调度器的主线程 其他GetThis()都是nullptr
                           // 哪个创建 哪个销毁
    t_scheduler = nullptr;
  }
}

Scheduler *Scheduler::GetThis() { return t_scheduler; }

Fiber *Scheduler::GetMainFiber() { return t_scheduler_fiber; }

// start 函数开辟线程
void Scheduler::start() {
  MutexType::Lock lock(m_mutex);
  if (!m_stopping) { // 现在应该是停止状态 所以m_stopping = true
    return;
  }
  m_stopping = false;
  SPADGER_ASSERT(m_threads.empty());

  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getTid());
  }
  lock.unlock();
}
void Scheduler::stop() {
  m_autoStop = true; // 这个变量有什么用
  // 如果只有一个线程 就是启动线程
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM ||
       m_rootFiber->getState() == Fiber::INIT)) {
    SPADGER_LOG_INFO(g_logger) << this << "stopped";
    m_stopping = true; // 只有一个线程的话直接设置stopping=true
    if (stopping()) { // 没有任务也没有再执行任务的线程就可以退出了
      return;
    }
  }

  // bool exit_on_this_fiber = false;
  if (m_rootThread != -1) { // use_caller
    SPADGER_ASSERT(GetThis() == this);
  } else { // m_rootThread == -1
    // not use_caller, so can be executed at any thread
    SPADGER_ASSERT(GetThis() !=
                   this); // 这种情况所有线程的t_scheduler不会设置,自然会成立了
  }

  m_stopping = true;
  // first. tickle 多次 存疑 tickle是为了什么 (需要实现才知道)
  for (size_t i = 0; i < m_threadCount; ++i) {
    tickle(); // notify all threads to stop
  }

  if (m_rootFiber) { // 如果有主协程 也要tickle
    tickle();
  }

  // 如果caller线程也需要run的话，就调用call()
  if (m_rootFiber) {
    if (!stopping()) // 没有完成停止
      m_rootFiber->call();
  }

  // second. join 等待所有线程结束
  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(m_mutex);
    thrs.swap(m_threads);
  }
  for (auto &i : thrs) {
    i->join();
  }
}

void Scheduler::setThis() { t_scheduler = this; }

void Scheduler::run() {
  /*
    1. 设置当前线程的scheduler
    2. 设置当前线程的main fiber 和 idle fiber
    3.从队列中取出任务
        1. 成功取出，执行
        2. 没有可执行任务，idle fiber运行等待被其他唤醒
  */
  SPADGER_LOG_DEBUG(g_logger) << "Task in scheduler is running...";
  set_hook_enable(true);
  //设置线程的t_scheduler(每个线程都有记录scheduler)
  setThis();
  // 对于每个非主线程 将其t_scheduler_fiber设置为线程自己的主协程
  if (spadger::GetThreadId() != m_rootThread) {
    t_scheduler_fiber =
        Fiber::GetThis()
            .get(); // 我寻思 这也没用啊 非主线程的t_scheduler_fiber完全没用
  }

  // 空闲协程 当没有任务时执行 也是虚函数需要自己实现
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;

  FiberAndThread ft; // 任务取出来存放的变量
  while (true) {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      // 这样scheduler只有一个m_fibers队列 竞争会不会不太好

      MutexType::Lock lock(m_mutex);
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        // 如果指定线程执行 但是又不是那个线程的话 就下一个
        if (it->thread != -1 && it->thread != spadger::GetThreadId()) {
          ++it;
          tickle_me = true; // 虽然自己不能处理 但是可以通知别人处理
          continue;
        }
        SPADGER_ASSERT(it->fiber || it->cb);
        // 正在执行就算了 问题来了 为什么会有这种情况?? 按道理来说
        // 应该是先取出来再执行的啊
        if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }

        ft = *it;
        m_fibers.erase(it++);
        ++m_activeThreadCount;
        is_active = true;
        break;
      }
      tickle_me |= it != m_fibers.end(); // 还有任务的话也需要唤醒一下别的线程
    }
    if (tickle_me) {
      tickle();
    }

    if (ft.fiber && ft.fiber->getState() != Fiber::TERM &&
        ft.fiber->getState() != Fiber::EXCEPT) {
      ft.fiber->swapIn();
      --m_activeThreadCount;
      // swapIn执行后 如果没有结束还是要重新放进队列里的
      if (ft.fiber->getState() == Fiber::READY) {
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::TERM &&
                 ft.fiber->getState() != Fiber::EXCEPT) {
        ft.fiber->setState(Fiber::HOLD); // 变成HOLD然后呢 也没个动静了
      }
      ft.reset();
    } else if (ft.cb) {
      if (cb_fiber) {
        cb_fiber->reset(ft.cb); // 重置fiber的function
      } else {
        cb_fiber.reset(new Fiber(ft.cb));
      }
      ft.reset(); // 指针置为空 因为使命已完成 不需要了
      // 开始swapIn
      cb_fiber->swapIn();
      --m_activeThreadCount;
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset(); // 共享指针置空
      } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                 cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr); // 一切都结束了
      } else {                    // HOLD &EXEC
        cb_fiber->setState(Fiber::HOLD);
        cb_fiber.reset(); // 共享指针置空
      }
    } else { // 没有取到任务执行，执行idle休眠 如果有任务的话 会被唤醒的
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }
      if (idle_fiber->getState() == Fiber::TERM) {
        SPADGER_LOG_INFO(g_logger) << "idle fiber term......";
        break;
      }
      ++m_idleThreadCount;
      idle_fiber->swapIn(); // 执行idle()的过程中被唤醒后跳出
      --m_idleThreadCount;
      if (idle_fiber->getState() != Fiber::TERM &&
          idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->setState(Fiber::HOLD); // READY EXEC -> HOLD
      }
    }
  }
}

void Scheduler::tickle() { SPADGER_LOG_INFO(g_logger) << "tickle"; }

bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  return m_autoStop && m_stopping && m_fibers.empty() &&
         m_activeThreadCount == 0;
}

void Scheduler::idle() {
  // 子类会实现的 这里没什么用
  SPADGER_LOG_INFO(g_logger) << "idle.........................";
  while (!stopping()) { // 没有任务搁着循环呢
    Fiber::YieldToHold();
  }
}

// 在某个线程执行时 代表将协程放在其他线程里
void Scheduler::switchTo(int thread) {
  SPADGER_ASSERT(Scheduler::GetThis() != nullptr);
  if (Scheduler::GetThis() ==
      this) { // 难道不是每个线程的t_scheduler都是scheduler吗
    if (thread == -1 || thread == spadger::GetThreadId()) {
      return;
    }
  }
  schedule(Fiber::GetThis(),
           thread); // 将当前线程的当前协程重新放入队列 并设置好调度的线程
  Fiber::YieldToHold(); // 让出执行权
}

std::ostream &Scheduler::dump(std::ostream &os) {
  MutexType::Lock lokc(m_mutex); // 输出之前当然要加锁
  os << "[Scheduler name=" << m_name << " size=" << m_threadCount
     << " active_count=" << m_activeThreadCount
     << " idle_count=" << m_idleThreadCount << " stopping=" << m_stopping
     << " ]" << std::endl
     << "   ";
  for (size_t i = 0; i < m_threads.size(); ++i) {
    if (i) {
      os << ", ";
    }
    os << m_threadIds[i];
  }
  return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler *target) {
  m_caller = Scheduler::GetThis();
  if (target) {
    target->switchTo();
  }
}

SchedulerSwitcher::~SchedulerSwitcher() {
  if (m_caller) {
    m_caller->switchTo();
  }
}

} // namespace spadger