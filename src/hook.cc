/*
 * @Author: lxk
 * @Date: 2022-10-16 12:01:40
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-22 19:43:02
 */
#include "hook.h"
#include "config.h"
#include "fd_manager.h"
#include "fiber.h"
#include "iomanager.h"
#include "log.h"
#include <dlfcn.h>

spadger::Logger::ptr g_logger = SPADGER_LOG_NAME("system");
namespace spadger {

static spadger::ConfigVar<int>::ptr g_tcp_connect_timeout =
    spadger::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX)                                                           \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(nanosleep)                                                                \
  XX(socket)                                                                   \
  XX(connect)                                                                  \
  XX(accept)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)                                                                    \
  XX(fcntl)                                                                    \
  XX(ioctl)                                                                    \
  XX(setsockopt)                                                               \
  XX(getsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }
  // 下面的XX定义了使用什么版本的func作为name_f，相当于是一个生产器
  // HOOK_FUN是一个工厂如果没有HOOK_FUN的话，每次都要把所有的函数写一遍
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX);
#undef XX
}

// 在main之前执行
static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener(
        [](const int &old_val, const int &new_val) {
          s_connect_timeout = new_val;
          SPADGER_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                     << old_val << " to " << new_val;
        });
  }
};
static _HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }

void set_hook_enable(bool flag) { t_hook_enable = flag; }

} // namespace spadger

struct timer_info {
  int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&... args) {
  if (!spadger::t_hook_enable) {
    return fun(fd, std::forward<Args>(args)...);
  }
  spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    return fun(fd, std::forward<Args>(args)...);
  }
  if (ctx->isClose()) {
    errno = EBADF;
    return -1;
  }
  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    return fun(fd, std::forward<Args>(args)...);
  }

  uint64_t to = ctx->getTimeout(timeout_so);
  std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
  // 尝试做原版同步的IO操作
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  //   SPADGER_LOG_DEBUG(g_logger) << hook_fun_name << " n:" << n;
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  // 如果原版的IO操作结果为EAGAIN，说明需要使用异步操作
  // 也就是iom->addEvent，这样不至于阻塞当前线程
  if (n == -1 && errno == EAGAIN) {
    SPADGER_LOG_INFO(g_logger) << "di_io<" << hook_fun_name << ">";
    spadger::IOManager *iom = spadger::IOManager::GetThis();
    spadger::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);

    // 如果有超时选项，那么在fd没有遇到event的时候可能会超时，
    // 所以需要addTimer(行为是在超时之后取消cancelEvent)
    if (to != (uint64_t)-1) {
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
              return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, (spadger::IOManager::Event)event);
          },
          winfo);
    }
    // 为fd添加event事件 使其成为异步事件
    // std::cout << "fd:" << fd << std::endl;
    int rt = iom->addEvent(fd, (spadger::IOManager::Event)(event));
    if (rt) {
      SPADGER_LOG_ERROR(g_logger)
          << hook_fun_name << " addEvent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      // 被(epoll)通知之后回到这里继续执行
      spadger::Fiber::YieldToHold();
      // 可能有两个原因回到这里:
      // 1. 正常返回: event事件发生 所以返回
      // 2. 超时
      // 如果定时器存在 就cancel，因为没用了

      if (timer) {
        timer->cancel();
      }
      // 如果异常 需要return -1
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      // 如果正常，retry继续fun(fd)) 因为fd已经准备好了
      goto retry; // why do{}while(flag) 在这里设置flag=true retry出设置为false
    }
  }
  return n;
}

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!spadger::t_hook_enable) {
    return sleep_f(seconds);
  }
  // 我们自己定义的函数不再使用标准库的函数，而是使用定时器，可以避免线程sleep
  // 化同步为异步
  // 以泡面店为例，每个线程都是一个工作人员，如果使用标准库的sleep，
  // 工作人员一次只能泡一份泡面，只能干等着，但是使用定时器的方式可以同时操作多份
  // 定时器到时会通知工作人员，工作人员对泡好的泡面做下一步动作即可，没必要干等着
  spadger::Fiber::ptr fiber = spadger::Fiber::GetThis();
  spadger::IOManager *iom = spadger::IOManager::GetThis();
  iom->addTimer(seconds * 1000,
                std::bind((void (spadger::IOManager::*)(spadger::Fiber::ptr,
                                                        int thread)) &
                              spadger::IOManager::schedule,
                          iom, fiber, -1));
  spadger::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!spadger::t_hook_enable) {
    return usleep_f(usec);
  }
  spadger::Fiber::ptr fiber = spadger::Fiber::GetThis();
  spadger::IOManager *iom = spadger::IOManager::GetThis();
  iom->addTimer(usec / 1000, std::bind((void (spadger::IOManager::*)(
                                           spadger::Fiber::ptr, int thread)) &
                                           spadger::IOManager::schedule,
                                       iom, fiber, -1));
  spadger::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
  if (!spadger::t_hook_enable) {
    return nanosleep_f(req, rem);
  }
  // 计算毫秒数
  uint64_t timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 * 1000;
  spadger::Fiber::ptr fiber = spadger::Fiber::GetThis();
  spadger::IOManager *iom = spadger::IOManager::GetThis();
  iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber); });
  spadger::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (!spadger::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  // 下面设置fd的属性(比如有么有timeout isSocket) 方便之后使用啊
  spadger::FdMgr::GetInstance()->get(fd, true);
  return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  if (!spadger::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClose()) {
    errno = EBADF;
    return -1;
  }

  if (!ctx->isSocket()) {
    return connect_f(fd, addr, addrlen);
  }

  if (ctx->getUserNonblock()) {
    return connect_f(fd, addr, addrlen);
  }

  // 开始连接
  int n = connect_f(fd, addr, addrlen);
  // 如果连接成功直接返回
  if (n == 0) {
    return 0;
  } else if (n != -1 || errno != EINPROGRESS) {
    return n;
  }
  // EINPROGRESS 的话就需要等待了哦 但是我们不阻塞等待呢 而是添加定时器和event
  // 之后Tield让出线程 干别的事情去吧
  // 1. 如果有事件发生 说明连接成功 回到这里检查返回
  // 2. 如果超时 timer会通知你的 需要返回-1 (超时是真没办法了)
  spadger::IOManager *iom = spadger::IOManager::GetThis();
  spadger::Timer::ptr timer;
  std::shared_ptr<timer_info> tinfo(new timer_info);
  std::weak_ptr<timer_info> winfo(tinfo);

  if (timeout_ms != (uint64_t)-1) {
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, fd, iom]() {
          auto t = winfo.lock();
          if (!t || t->cancelled) {
            return;
          }
          t->cancelled = ETIMEDOUT;
          iom->cancelEvent(fd, spadger::IOManager::WRITE);
        },
        winfo);
  }

  int rt = iom->addEvent(fd, spadger::IOManager::WRITE);
  if (rt == 0) {
    spadger::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer) {
      timer->cancel();
    }
    SPADGER_LOG_ERROR(g_logger)
        << "connect addEvent(" << fd << ", WRITE) error";
  }

  int error = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  if (!error) {
    return 0;
  } else {
    errno = error;
    return -1;
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen,
                              spadger::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = do_io(sockfd, accept_f, "accept", spadger::IOManager::READ,
                 SO_RCVTIMEO, addr, addrlen); // addr addrlen原有参数不变
  if (fd >= 0) {
    spadger::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
  return do_io(fd, read_f, "read", spadger::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", spadger::IOManager::READ, SO_RCVTIMEO, iov,
               iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  //   return recv_f(sockfd, buf, len, flags);
  return do_io(sockfd, recv_f, "recv", spadger::IOManager::READ, SO_RCVTIMEO,
               buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return do_io(sockfd, recvfrom_f, "recvfrom", spadger::IOManager::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", spadger::IOManager::READ,
               SO_RCVTIMEO, msg, flags);
}
ssize_t write(int fd, const void *buf, size_t count) {
  return do_io(fd, write_f, "write", spadger::IOManager::WRITE, SO_SNDTIMEO,
               buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", spadger::IOManager::WRITE, SO_SNDTIMEO,
               iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
  return do_io(s, send_f, "send", spadger::IOManager::WRITE, SO_SNDTIMEO, msg,
               len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen) {
  return do_io(s, sendto_f, "sendto", spadger::IOManager::WRITE, SO_SNDTIMEO,
               msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
  return do_io(s, sendmsg_f, "sendmsg", spadger::IOManager::WRITE, SO_SNDTIMEO,
               msg, flags);
}
int close(int fd) {
  if (!spadger::t_hook_enable) {
    return close_f(fd);
  }
  // 先取消iom的设置和FdManager中的设置
  spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
  if (ctx) {
    auto iom = spadger::IOManager::GetThis();
    if (iom) {
      iom->cancelAll(fd);
    }
    spadger::FdMgr::GetInstance()->del(fd);
  }
  return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */) {
  if (!spadger::t_hook_enable) {
    return fcntl_f(fd, cmd);
  }

  va_list va;
  va_start(va, cmd);
  switch (cmd) {
  case F_SETFL: {
    int arg = va_arg(va, int);
    va_end(va);
    spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
      return fcntl_f(fd, cmd, arg);
    }
    // 把参数的NONBLOCK设置到userNONblock上
    ctx->setUserNonblock(arg & O_NONBLOCK);
    // 但是具体 有没有NONBLOCK 还是需要看sysNonblock的
    if (ctx->getSysNonblock()) {
      arg |= O_NONBLOCK;
    } else {
      arg &= ~O_NONBLOCK;
    }
    return fcntl_f(fd, cmd, arg);
  } break;

  case F_GETFL: {
    va_end(va);
    int arg = fcntl_f(fd, cmd);
    spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
      return arg;
    }
    //获取还不够 因为user的NONBLOCK设置在FdCtx中，所以需要加上
    if (ctx->getUserNonblock()) {
      return arg | O_NONBLOCK;
    } else {
      return arg & ~O_NONBLOCK;
    }
  } break;
    // 根据参数类型划分
  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  case F_SETFD:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
  case F_SETPIPE_SZ: {
    int arg = va_arg(va, int);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETFD:
  case F_GETOWN:
  case F_GETSIG:
  case F_GETLEASE:
  case F_GETPIPE_SZ: {
    va_end(va);
    return fcntl_f(fd, cmd);
  } break;
  case F_SETLK:
  case F_SETLKW:
  case F_GETLK: {
    struct flock *arg = va_arg(va, struct flock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETOWN_EX:
  case F_SETOWN_EX: {
    struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  default:
    va_end(va);
    return fcntl_f(fd, cmd);
    break;
  }
}

int ioctl(int fd, unsigned long request, ...) {
  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);

  if (request == FIONBIO) {
    bool user_nonblock = !!*(int *)arg; // 转变为0/1
    spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose() || !ctx->isSocket()) {
      return ioctl_f(fd, request, arg);
    }
    ctx->setUserNonblock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}
int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (!spadger::t_hook_enable) {
    return setsockopt_f(sockfd, level, optname, optval, optlen);
  }
  // 我们只负责socket的设置
  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      spadger::FdCtx::ptr ctx = spadger::FdMgr::GetInstance()->get(sockfd);
      if (ctx) {
        const timeval *tv = (const timeval *)optval;
        ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}
}
