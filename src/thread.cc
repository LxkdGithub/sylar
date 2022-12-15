/*
 * @Author: lxk
 * @Date: 2021-12-17 16:02:06
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-05 19:41:41
 */

#include "thread.h"
#include "log.h"
#include "util.h"
#include <iostream>

namespace spadger {

Semaphore::Semaphore(uint32_t count) {
  if (sem_init(&m_semaphore, 0, count)) {
    throw std::logic_error("sem_init error");
  }
}
Semaphore::~Semaphore() { sem_destroy(&m_semaphore); }
void Semaphore::wait() {
  if (sem_wait(&m_semaphore)) {
    throw std::logic_error("sem_wait error");
  }
}
void Semaphore::notify() {
  if (sem_post(&m_semaphore)) {
    throw std::logic_error("sem_post error");
  }
}

// Think: how to implement pthread_self()??
static thread_local Thread *t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb), m_name(name) {
  if (name.empty()) {
    m_name = "UNKNOW";
  }
  int res = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (res) {
    std::cout << "pthread_create failed" << std::endl;
  }
  m_semaphore.wait();
}

Thread::~Thread() {
  if (m_thread) {
    pthread_detach(m_thread);
  }
}

void Thread::join() {
  if (m_thread) {
    int res = pthread_join(m_thread, nullptr);
    if (res) {
      std::cout << "pthread_join failed" << std::endl;
    }
    m_thread = 0;
  }
}

// Add a layer/wrap to get the tid in the process of thread.
void *Thread::run(void *arg) {
  // Set the thread_local variable:t_thread object(id and name)
  Thread *thread = (Thread *)arg;
  thread->m_id = GetThreadId();
  t_thread = thread;
  // Set the thread name in system(we can see top -H - p processId)
  pthread_setname_np(pthread_self(),
                     thread->m_name.substr(0, 16).c_str()); // 最长16字节

  std::function<void()> cb;
  cb.swap(thread->m_cb);

  thread->m_semaphore.notify();
  cb();
  return 0;
}

// When thread is running, GetThis() can return corresponding Thread object;
Thread *Thread::GetThis() { return t_thread; }

const std::string &Thread::GetName() {
  if (t_thread) {
    return t_thread->m_name;
  }
  return t_thread_name;
}

void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

} // namespace spadger