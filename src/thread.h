/*
 * @Author: lxk
 * @Date: 2021-12-17 15:42:28
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-21 18:01:20
 */
#ifndef __SPADGER_THREAD_H__
#define __SPADGER_THREAD_H__

#include "mutex.h"
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>
// #include <thread>

namespace spadger {

// C++可以使用C API的semaphore进行包装 也可以使用mutex和condition_variable实现
class Semaphore {
public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();
  void wait();
  void notify();

private:
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

private:
  sem_t m_semaphore;
};

class Thread {
public:
  typedef std::shared_ptr<spadger::Thread> ptr;
  Thread(std::function<void()> cb, const std::string &name);
  ~Thread();

  // get set methods
  const std::string &getName() const { return m_name; }
  pid_t getTid() const { return m_id; }

  // work funciton
  void join();

  static Thread *GetThis();                     // thread_local variable
  static const std::string &GetName();          // thread_local variable
  static void SetName(const std::string &name); // set thread_local variable

private:
  // std::thread has thread(thread&& other) method.
  Thread(const Thread &) = delete;
  Thread(const Thread &&) = delete;
  Thread &operator=(const Thread &) = delete;

  static void *run(void *arg);

private:
  pid_t m_id = -1;
  pthread_t m_thread = 0;
  std::function<void()> m_cb;
  std::string m_name;

  Semaphore m_semaphore; // Ensure the thread is running when the Thread was
                         // constructed.
};

} // end namespace spadger
#endif
