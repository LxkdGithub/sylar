#include "iomanager.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
namespace spadger {

static spadger::Logger::ptr g_logger = SPADGER_LOG_NAME("system");

IOManager::FdContext::EventContext &
IOManager::FdContext::getContext(IOManager::Event event) {
  switch (event) {
  case IOManager::READ:
    return read;
  case IOManager::WRITE:
    return write;
  default:
    SPADGER_ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invalid event");
}
void IOManager::FdContext::resetContext(
    IOManager::FdContext::EventContext &ctx) {
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event) {
  SPADGER_ASSERT(events & event); // 确保设置了这个事件
  events = (Event)(events & ~event);
  EventContext &ctx = getContext(event);
  if (ctx.cb) {
    ctx.scheduler->schedule(&ctx.cb);
  } else {
    ctx.scheduler->schedule(&ctx.fiber);
  }
  ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
    : Scheduler(threads, use_caller, name) {
  m_epfd = epoll_create(5000);
  SPADGER_ASSERT(m_epfd > 0);

  int rt = pipe(m_tickleFds); // return 0 if successful
  SPADGER_ASSERT(rt == 0);

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN | EPOLLET; // 边沿触发
  event.data.fd = m_tickleFds[0];

  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  SPADGER_ASSERT(rt == 0);
  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  SPADGER_ASSERT(rt == 0);

  contextResize(32);
  start();
}

IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);
  for (size_t i = 0; i < m_fdContexts.size(); i++) {
    if (m_fdContexts[i]) {
      delete m_fdContexts[i];
    }
  }
}

// =================================================
// IOManager cibtextResize
// =================================================
void IOManager::contextResize(size_t size) {
  m_fdContexts.resize(size);
  for (size_t i = 0; i < m_fdContexts.size(); i++) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

// =================================================
// IOManager addEvent
// =================================================
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext *fd_ctx = nullptr;
  RWMutexType::ReadLock lock(m_mutex);
  if (fd < (int)m_fdContexts.size()) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    // need to resize
    lock.unlock();
    RWMutexType::WriteLock lock2(m_mutex);
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutexType::Lock lock2(fd_ctx->mutex);
  if (fd_ctx->events & event) { // unlikely
    SPADGER_LOG_ERROR(g_logger)
        << "addEvent assert fd=" << fd << " event=" << event
        << " fd_ctx.event=" << fd_ctx->events;
    SPADGER_ASSERT(!(fd_ctx->events & event));
  }

  int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event ep_event;
  ep_event.data.ptr = fd_ctx; // 第一次使用 随你放什么指针
  ep_event.events = EPOLLET | fd_ctx->events | event;
  int rt = epoll_ctl(m_epfd, op, fd, &ep_event);
  if (rt) { // return 0 if successful
    SPADGER_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
        << ep_event.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return -1;
    ;
  }
  ++m_pendingEventCount;
  // 修改fd_ctx的event
  fd_ctx->events = Event(fd_ctx->events | event);
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  SPADGER_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
  event_ctx.scheduler = Scheduler::GetThis();
  if (cb) {
    event_ctx.cb.swap(cb);
  } else {
    event_ctx.fiber = Fiber::GetThis();
    SPADGER_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
  }
  return 0;
}

// =================================================
// IOManager delEvent
// =================================================
bool IOManager::delEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (fd >= static_cast<int>(m_fdContexts.size())) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock1(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) { // unlikely
    return false;
  }

  Event new_events = (Event)(fd_ctx->events & ~event);
  epoll_event ep_event;
  ep_event.events = new_events | EPOLLET;
  ep_event.data.ptr = fd_ctx;
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  int rt = epoll_ctl(m_epfd, op, fd, &ep_event);
  if (rt) {
    SPADGER_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
        << ep_event.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }
  --m_pendingEventCount;
  // cancel直接就去除了，没有像cancel那样还要trigger
  fd_ctx->events = new_events;
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx); // 直接置为空即可
  return true;
}

// =================================================
// IOManager cancelEvent
// =================================================
bool IOManager::cancelEvent(int fd, Event event) {
  RWMutexType::ReadLock lock(m_mutex);
  if (fd >= static_cast<int>(m_fdContexts.size())) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock1(fd_ctx->mutex);
  if (!(fd_ctx->events & event)) { // ulikely
    return false;
  }
  Event new_events = (Event)(fd_ctx->events & ~event);
  epoll_event ep_event;
  ep_event.data.ptr = fd_ctx;
  ep_event.events = new_events | EPOLLET;
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  int rt = epoll_ctl(m_epfd, op, fd, &ep_event);
  if (rt) {
    SPADGER_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
        << ep_event.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }
  fd_ctx->triggerEvent(event);
  --m_pendingEventCount;
  return true;
}

// =================================================
// IOManager 删除所有event (从m_epfd 和 fd_ctx->events)
// =================================================
bool IOManager::cancelAll(int fd) {
  RWMutexType::ReadLock lock(m_mutex);
  if (fd >= static_cast<int>(m_fdContexts.size())) {
    return false;
  }
  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock1(fd_ctx->mutex);
  if (!fd_ctx->events) {
    return false;
  }

  // 去除m_epfd中的event，并且最后triggerEvent，最后再schedule一次
  // 正常的流程也是epoll_wait之后去除m_epfd中的event，并且schedule
  int op = EPOLL_CTL_DEL;
  epoll_event ep_event;
  ep_event.data.ptr = fd_ctx;
  ep_event.events = 0;
  int rt = epoll_ctl(m_epfd, op, fd, &ep_event);
  if (rt) {
    SPADGER_LOG_ERROR(g_logger)
        << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
        << ep_event.events << "):" << rt << " (" << errno << ") ("
        << strerror(errno) << ")";
    return false;
  }
  // fd_ctx triggeEvent在schedule之后也从fd_ctx->events删除event了
  if (fd_ctx->events & READ) {
    fd_ctx->triggerEvent(READ);
    --m_pendingEventCount;
  }
  if (fd_ctx->events & WRITE) {
    fd_ctx->triggerEvent(WRITE);
    --m_pendingEventCount;
  }
  SPADGER_ASSERT(fd_ctx->events == 0);
  return true;
}
IOManager *IOManager::GetThis() { // static function return
  return dynamic_cast<IOManager *>(Scheduler::GetThis()); // TODO:?
}

// =================================================
// IOManager 空闲时执行函数 // TODO: ?
// =================================================
void IOManager::tickle() {
  if (!hasIdleThreads()) {
    return;
  }
  int rt = write(m_tickleFds[1], "1", 1);
  SPADGER_ASSERT(rt == 1); // 返回长度为1
}

// =================================================
// IOManager 是否停止
// =================================================
bool IOManager::stopping(uint64_t &timeout) {
  timeout = getNextTimer();
  return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

bool IOManager::stopping() {
  uint64_t timeout = 0;
  return stopping(timeout);
}

// =================================================
// IOManager idle函数
// =================================================
void IOManager::idle() {
  // 初始化epoll_event数组用于存放epoll_wait结果
  epoll_event *events = new epoll_event[64]();
  std::shared_ptr<epoll_event> shared_events(
      events, [](epoll_event *ptr) { delete[] ptr; });
  while (true) {
    uint64_t next_timeout = 0;
    if (stopping(next_timeout)) {
      SPADGER_LOG_INFO(g_logger)
          << "name=" << getName() << " idle stopping exit.";
      break;
    }
    int rt = 0;
    do {
      // 通过控制时间来达到timer和IO event合并的目的
      static const int MAX_TIMEOUT = 3000;
      if (next_timeout == ~0ull) {
        next_timeout = MAX_TIMEOUT;
      } else {
        next_timeout =
            (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
      }
      rt = epoll_wait(m_epfd, events, 64, int(next_timeout));
      if (rt < 0 && errno == EINTR) {
        // 如果是因为中断的就继续
      } else {
        // 其他情况
        // 1. 非中断的错误
        // 2. 超时
        // 3. 有ready requets IO
        break;
      }
    } while (true);

    // 1. 首先获取定时器列表中的过期cbs并执行
    std::vector<std::function<void()>> cbs;
    listExpiredCb(cbs);
    if (!cbs.empty()) {
      schedule(cbs.begin(), cbs.end());
      cbs.clear();
    }

    // 2. 之后是IO evnet的处理
    for (int i = 0; i < rt; i++) {
      epoll_event &event = events[i];
      // 1. 首先判断是否是tickleFds[0]
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy[256];
        int r;
        int flag = fcntl(m_tickleFds[0], F_GETFL) | O_NONBLOCK;
        fcntl(m_tickleFds[0], F_SETFL, flag);
        while ((r = read(m_tickleFds[0], dummy, sizeof(dummy))) > 0) {
          //   std::cout << r << " " << dummy << std::endl;
        }
        continue; // 如果是通知的话，知道就行了
      }

      FdContext *fd_ctx = (FdContext *)(event.data.ptr);
      FdContext::MutexType::Lock lock(fd_ctx->mutex);
      if (event.events & (EPOLLHUP | EPOLLERR)) {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
      }
      // 转化 EPOLLXX -> READ WRITE
      int real_events = NONE;
      if (event.events & EPOLLIN) {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT) {
        real_events |= WRITE;
      }
      if ((Event)(real_events & fd_ctx->events) == NONE) { // 与等待的事件不对应
        continue;
      }
      int left_events = (fd_ctx->events & ~real_events);
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
      if (rt2) {
        SPADGER_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd_ctx->fd << ","
            << event.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        continue;
      }

      if (real_events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
      }
      if (real_events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
      }
    }
    // Idle函数收到消息之后 就需要swapOut回到scheduler_fiber
    Fiber::ptr cur = Fiber::GetThis();
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
  }
}

void IOManager::onTimerInsertedAtFront() {
  tickle(); // 唤醒
}

} // namespace spadger