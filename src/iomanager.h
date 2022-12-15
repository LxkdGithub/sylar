/*
 * @Author: lxk
 * @Date: 2022-10-13 17:38:26
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-16 11:25:48
 */

#ifndef __SPADGER_IOMANAGER_H__
#define __SPADGER_IOMANAGER_H__
#include "scheduler.h"
#include "timer.h"
#include <atomic>
#include <sys/epoll.h>

namespace spadger {
class IOManager : public Scheduler, public TimerManager {
public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  enum Event { NONE = 0x0, READ = 0x1, WRITE = 0x4 };

private:
  struct FdContext {
    typedef Mutex MutexType;
    struct EventContext {
      Scheduler *scheduler = nullptr; // 事件执行的scheduler
      Fiber::ptr fiber;               // 事件的协程
      std::function<void()> cb;
    };

    EventContext &getContext(Event event);
    void resetContext(EventContext &ctx);
    void triggerEvent(Event event);

    EventContext read;   // 读事件如何处理
    EventContext write;  // 写事件如何处理
    int fd;              // 句柄
    Event events = NONE; // 已经注册的事件
    MutexType mutex;
  };

public:
  IOManager(size_t threads = 1, bool use_caller = true,
            const std::string &name = "");
  ~IOManager();

  // 为fd的事件添加处理函数
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  bool delEvent(int fd, Event event);
  bool cancelEvent(int fd, Event event);
  bool cancelAll(int fd);
  static IOManager *GetThis();

protected:
  // 从Scheduler继承的虚函数
  void tickle() override;
  void idle() override;
  bool stopping();
  bool stopping(uint64_t &timeout);

  void contextResize(size_t size);
  void onTimerInsertedAtFront() override;

private:
  int m_epfd;
  int m_tickleFds[2]; // pair for fd to notify.

  std::atomic<size_t> m_pendingEventCount = {0};
  RWMutexType m_mutex;
  std::vector<FdContext *> m_fdContexts;
};

} // namespace spadger

#endif