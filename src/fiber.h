/*
 * @Author: lxk
 * @Date: 2021-12-24 10:36:18
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 17:23:57
 */

#ifndef __SPADGER_FIBER_H__
#define __SPADGER_FIBER_H__

#include <functional>
#include <memory>
#include <ucontext.h>

namespace spadger {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

public:
  typedef std::shared_ptr<Fiber> ptr;

  enum State {
    INIT,
    HOLD,
    EXEC,
    TERM,
    READY,
    EXCEPT,
  };

private:
  // generate main thread
  Fiber();

public:
  Fiber(std::function<void()> cb, size_t stacksize = 0,
        bool use_caller = false);
  ~Fiber();

  // 重置协程函数和协程状态
  void reset(std::function<void()> cb);

  // 切换到前台执行
  void swapIn();
  // 切换到后台执行
  void swapOut();

  /**
   * @brief 将当前线程切换到执行状态
   * @pre 执行的为当前线程的主协程
   */
  void call();

  /**
   * @brief 将当前线程切换到后台
   * @pre 执行的为该协程
   * @post 返回到线程的主协程
   */
  void back();

  uint64_t getId() const { return m_id; }

  State getState() { return m_state; }
  void setState(State state) { m_state = state; }

  // static method (负责的是线程内的协程状态 当前协程这些t_fiber)
public:
  // 当前协程
  void SetThis(Fiber *f);
  static Fiber::ptr GetThis();
  // 切换到后台 设置为ready状态
  static void YieldToReady();
  // 切换到后台 设置为Hold状态
  static void YieldToHold();

  static uint64_t TotalFibers();

  static void MainFunc();

  static uint64_t GetFiberId();

  static void CallerMainFunc();

private:
  uint64_t m_id = 0;
  uint32_t m_stacksize = 0;
  State m_state = INIT;

  ucontext_t m_ctx;
  void *m_stack = nullptr; // 自己在函数实现栈的reload和save

  std::function<void()> m_cb;
};

} // namespace spadger

#endif
