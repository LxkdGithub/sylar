/*
 * @Author: lxk
 * @Date: 2021-12-26 10:38:59
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-20 16:15:33
 */
#include "fiber.h"
#include "config.h"
#include "scheduler.h"
#include "util.h"
#include <atomic>

namespace spadger {

static Logger::ptr g_logger = SPADGER_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

// current fiber
static thread_local Fiber *t_fiber = nullptr;
// main fiber in thread
static thread_local Fiber::ptr t_threadFiber = nullptr;

// Lookup is a static method
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 1024 * 1024, "fiber stack size");

class MllocStackAllocator {
public:
  static void *Alloc(size_t size) { return malloc(size); }
  static void Dealloc(void *vp, size_t size) {
    return free(vp); // 为什么return
  }
};

// 可以切换其他的栈内存分配方式
using StackAllocator = MllocStackAllocator;

uint64_t Fiber::GetFiberId() {
  // 如果使用GetThis 的话在没有协程时就会构造main fiber，没必要
  if (t_fiber) {
    return t_fiber->getId();
  }
  return 0;
}

// 主协程构造 在静态成员函数中调用(GetThis())
Fiber::Fiber() {
  m_state = EXEC;
  SetThis(this); // 将当前fiber 放入thread_local fiber object

  // ucontext 线程主协程初始化
  if (getcontext(&m_ctx)) {
    SPADGER_ASSERT2(false, "getcontext");
  }
  ++s_fiber_count;

  SPADGER_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// 非main Fiber 需要分配栈空间 和 cb
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
  m_stack = StackAllocator::Alloc(m_stacksize);

  if (getcontext(&m_ctx)) {
    SPADGER_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  m_ctx.uc_link = nullptr;

  if (!use_caller) {
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
  }
  SPADGER_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}
Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    // 不是主协程
    SPADGER_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    StackAllocator::Dealloc(m_stack, m_stacksize);
  } else {
    // 没有 cb和stack就是main fiber
    SPADGER_ASSERT(!m_cb);
    SPADGER_ASSERT(m_state == EXEC);

    Fiber *cur = t_fiber;
    if (cur == this) {
      SetThis(nullptr);
    }
  }
  SPADGER_LOG_DEBUG(g_logger)
      << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
}

// 重置协程函数和协程状态
void Fiber::reset(std::function<void()> cb) {
  // 成为新的协程
  SPADGER_ASSERT(m_stack); // 非主协程 有栈才可以
  SPADGER_ASSERT(m_state == TERM || m_state == INIT ||
                 m_state == EXCEPT); // 运行或者ready或者HOLD肯定不行

  m_cb = cb;
  if (getcontext(&m_ctx)) {
    SPADGER_ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link = nullptr; // 为什么不设置为main fiber
  m_ctx.uc_stack.ss_size = m_stacksize;
  m_ctx.uc_stack.ss_sp = m_stack;

  makecontext(&m_ctx, &Fiber::MainFunc,
              0); // 这里因为function<void()> 所以成员函数不行 只能是static函数
  m_state = INIT;
}

// =========================================================================
// recall和swapIn的区别在于 线程主协程还是调度器主协程 back和swapOut也是一样的
// 相当于是否要和调度器一起配合使用的选择

// 为什么swapOut和back不需要设置状态 因为有ready和hold两种状态
// 所以需要YieldToReady和YieldToHold两个函数来改变状态
// =========================================================================

void Fiber::call() {
  SetThis(this);
  m_state = EXEC;
  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
    SPADGER_ASSERT2(false, "swapcontext");
  }
}

// back
void Fiber::back() {
  SetThis(t_threadFiber.get());
  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    SPADGER_ASSERT2(false, "swapcontext");
  }
}

// ================================================================
// 切换只能在主协程和非主协程之间进行
// 在主协程状态将非主协程切换到前台执行
void Fiber::swapIn() {
  SetThis(this); // 操作时一定是主协程的
  SPADGER_ASSERT(m_state != EXEC);
  m_state = EXEC;
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx,
                  &m_ctx)) { // 搞清楚从哪到哪 左到右 old->new
    SPADGER_ASSERT2(false, "swapcontext");
  }
}
// 当前非主协程切换到后台执行
void Fiber::swapOut() {
  SetThis(Scheduler::GetMainFiber());
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    SPADGER_ASSERT2(false, "swapcontext");
  }
}

// -----------------------------------------------------------------
void Fiber::SetThis(Fiber *f) { t_fiber = f; }

Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this(); // 为什么不是t_fiber
  }
  //连主协程都没有的时候
  // Fiber()是私有的，只在线程还没有协程的时候创建主协程并设置t_fiber用
  Fiber::ptr main_fiber(new Fiber);
  SPADGER_ASSERT(t_fiber == main_fiber.get());
  t_threadFiber = main_fiber; // 设置线程主协程
  return t_fiber->shared_from_this();
}

// -------------------------------------------------------------------
// 切换到后台 设置为ready状态
void Fiber::YieldToReady() {
  // 设置状态 然后swapOut
  Fiber::ptr cur = GetThis();
  SPADGER_ASSERT(cur->getState() == State::EXEC);
  cur->m_state = READY;
  cur->swapOut();
}
// 切换到后台 设置为Hold状态
void Fiber::YieldToHold() {
  // 设置状态 然后swapOut
  Fiber::ptr cur = GetThis();
  SPADGER_ASSERT(cur->getState() == State::EXEC);
  //   cur->m_state = HOLD;
  cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
  // 此处应该加锁
  return s_fiber_count;
}
// ============================================================================

void Fiber::MainFunc() {
  Fiber::ptr cur = GetThis();
  SPADGER_ASSERT(cur);
  try {
    cur->m_cb();         // 执行
    cur->m_cb = nullptr; // 执行完置空
    cur->m_state = TERM; // 结束
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    SPADGER_LOG_ERROR(g_logger) << "Fiber except";
  } catch (...) {
    cur->m_state = EXCEPT;
    SPADGER_LOG_ERROR(g_logger) << "Fiber Except";
  }
  // cb执行完成后 还要回到主协程
  auto raw_cur = cur.get();
  cur.reset();
  raw_cur->swapOut();
}

void Fiber::CallerMainFunc() {
  Fiber::ptr cur = GetThis();
  SPADGER_ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception &ex) {
    cur->m_state = EXCEPT;
    SPADGER_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                                << " fiber_id=" << cur->getId() << std::endl
                                << spadger::BacktraceToString();
  } catch (...) {
    cur->m_state = EXCEPT;
    SPADGER_LOG_ERROR(g_logger) << "Fiber Except"
                                << " fiber_id=" << cur->getId() << std::endl
                                << spadger::BacktraceToString();
  }
  // 为什么下面要这样做
  // 因为如果不释放掉智能指针的话，上下文切换回主协程就永远回不来了，因为智能指针的关系，对象得不到释放
  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->back();
  SPADGER_ASSERT2(false,
                  "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

} // namespace spadger