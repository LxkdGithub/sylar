/*
 * @Author: lxk
 * @Date: 2021-12-26 21:34:41
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-16 11:32:17
 */

#ifndef __SPADGER_SCHEDULER_H___
#define __SPADGER_SCHEDULER_H___

#include "fiber.h"
#include "log.h"
#include "mutex.h"
#include "thread.h"
#include <atomic>
#include <list>
#include <memory>
#include <vector>

namespace spadger {

class Scheduler {
public:
  /// @brief Construct a scheduler
  /// @param threads: count of threads
  /// @param use_caller: whether to use the caller thread as worker thread.
  /// @param name: scheduler name
  Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");
  virtual ~Scheduler();

  const std::string &getName() const { return m_name; }

  static Scheduler *GetThis();
  static Fiber *GetMainFiber(); // 调度器也需要主协程

  void start();
  void stop();
  std::ostream &dump(std::ostream &);
  void switchTo(int thread = -1); // 默认没有指定 没有指定 就 不操作

  // 单个加入
  template <typename FiberOrCb> void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle;
    {
      MutexType::Lock lock(m_mutex);
      need_tickle = scheduleNoLock(fc, thread);
    }
    if (need_tickle) {
      tickle();
    }
  }

  // 使用迭代器加入
  template <typename InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
        begin++;
      }
    }
    if (need_tickle) {
      tickle();
    }
  }

protected:
  virtual void tickle(); // notify
  void run();
  virtual bool stopping(); // check if stopped
  virtual void idle();     // 没有发任务的时候怎么处理
  void setThis();          // 设置线程的scheduler为this
  bool hasIdleThreads() { return m_idleThreadCount > 0; }

private:
  template <typename FiberOrCb> bool scheduleNoLock(FiberOrCb fc, int thread) {
    // 判断原来协程队列是否是空的 用于唤醒执行
    bool need_tickle = m_fibers.empty();
    FiberAndThread ft(fc, thread);
    if (ft.cb || ft.fiber) {
      m_fibers.push_back(ft);
    }
    return need_tickle;
  }

private:
  struct FiberAndThread {
    Fiber::ptr fiber;
    std::function<void()> cb;
    int thread; // 指定在哪一个线程执行

    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}
    FiberAndThread(Fiber::ptr *f, int thr) : thread(thr) {
      fiber.swap(*f); // shared_ptr整体swap 防止出现引用释放问题
    }
    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}
    FiberAndThread(std::function<void()> *f, int thr) : thread(thr) {
      cb.swap(*f); // 图啥呀???
    }
    FiberAndThread() : thread(-1) {}

    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

private:
  MutexType m_mutex;                  // 锁
  std::vector<Thread::ptr> m_threads; // 线程池
  std::list<FiberAndThread> m_fibers; // 协程池
  Fiber::ptr m_rootFiber; // 调度器主协程 (use_caller为true才有用)
  std::string m_name;     // 调度器名称

protected:
  std::vector<int> m_threadIds;
  size_t m_threadCount = 0; // 主线程之外还有几个线程
  std::atomic<size_t> m_activeThreadCount = {0};
  std::atomic<size_t> m_idleThreadCount = {0};
  bool m_stopping = true; // 初始状态为停止状态
  bool m_autoStop = false;
  // m_rootThread是主线程的ID 如果use_caller=false的话，就没有这个所谓的主线程了
  int m_rootThread = 0;
};

class SchedulerSwitcher : public Noncopyable {
public:
  SchedulerSwitcher(Scheduler *target = nullptr);
  ~SchedulerSwitcher();

private:
  Scheduler *m_caller;
};

} // namespace spadger

#endif