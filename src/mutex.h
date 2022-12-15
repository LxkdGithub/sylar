/*
 * @Author: lxk
 * @Date: 2021-12-12 14:46:29
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-10 17:39:44
 */

#ifndef __SPADGER_MUTEX_H__
#define __SPADGER_MUTEX_H__

#include "noncopyable.h"
#include <boost/noncopyable.hpp>

// C API 拥有thread mutex rwmutex spinlock semaphore
#include <pthread.h>   // pthread_xxxactionxxx
                       // pthread_mutex_t -> pthread_mutex_xxx
                       // pthread_rwlock_t -> pthread_rwlock_xxx
                       // pthread_spinlock_t
#include <semaphore.h> // sema_t
// C++ 使用mutex和condition_variable实现信号量机制
// #include <condition_variable>
// #include <mutex>

#include <atomic>

namespace spadger {

// 实现信号量可以使用C-API 也可以自己使用mutex和condition_variable实现
// class Semaphore : boost::noncopyable
// {
// public:
//     Semaphore() {}
//     ~Semaphore() {}
//     void wait();
//     void nority();
// private:
//     sem_t m_semaphore;
// };

// class Semaphore
// {
// public:
//     Semaphore():m_count(0) {}
//     ~Semaphore() {}
//     void wait() {
//         std::unique_lock<std::mutex> lock(m_mutex);
//         // 此处的wait函数是一个封装 其实还是一个while判断结构
//         m_cv.wait(lock, [=]() { return m_count > 0;});
//         m_count--;
//     }
//     void notify() {
//         std::unique_lock<std::mutex> lock(m_mutex);
//         ++m_count;
//         m_cv.notify_one();
//     }

// private:
//     std::mutex m_mutex;
//     std::condition_variable m_cv;
//     int m_count;
// };

template <typename T> class ScopedLockImpl;

template <typename T> class ReadScopedLockImpl;

template <typename T> class WriteScopedLockImpl;

// 自己使用C-API写的mutex和rwMutex 为什么不使用C++自己的呢
class Mutex : Noncopyable {
public:
  typedef ScopedLockImpl<Mutex> Lock;
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }
  void lock() { pthread_mutex_lock(&m_mutex); }
  void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
  pthread_mutex_t m_mutex;
};

class RWMutex {
public:
  typedef ReadScopedLockImpl<RWMutex> ReadLock;
  typedef WriteScopedLockImpl<RWMutex> WriteLock;
  RWMutex() {
    pthread_rwlock_init(&m_lock, nullptr); // 后边参数是attr
  }
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }
  void wrlock() { pthread_rwlock_wrlock(&m_lock); }
  void rdlock() { pthread_rwlock_rdlock(&m_lock); }
  void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
  pthread_rwlock_t m_lock;
};

// LockGuard  或者说 ScopedLock  与C++ lock_guard类似
template <typename T> class ScopedLockImpl {
public:
  ScopedLockImpl(T &mutex) : m_mutex(mutex) {
    mutex.lock();
    m_locked = true;
  }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

  ~ScopedLockImpl() { unlock(); }

private:
  T &m_mutex; // mutex is not allowed to be copyed, so we can only use
              // reference.
  bool m_locked;
};

// 读锁模板实现 为什么单独写读锁和写锁呢 是因为这种lock_guard
// 基本收拾构造和析构进行加锁和解锁的
// 并且即使是手动控制锁也只能应用一种锁(读或者写) 为什么呢 因为防止出错
// 读和写的加锁解锁过程混淆
template <class T> struct ReadScopedLockImpl {
public:
  /**
   * @brief 构造函数
   * @param[in] mutex 读写锁
   */
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }

  /**
   * @brief 析构函数,自动释放锁
   */
  ~ReadScopedLockImpl() { unlock(); }

  /**
   * @brief 上读锁
   */
  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  /**
   * @brief 释放锁
   */
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  /// mutex
  T &m_mutex;
  /// 是否已上锁
  bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 */
template <class T> struct WriteScopedLockImpl {
public:
  /**
   * @brief 构造函数
   * @param[in] mutex 读写锁
   */
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }

  /**
   * @brief 析构函数
   */
  ~WriteScopedLockImpl() { unlock(); }

  /**
   * @brief 上写锁
   */
  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  /**
   * @brief 解锁
   */
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  /// Mutex
  T &m_mutex;
  /// 是否已上锁
  bool m_locked;
};

class Spinlock : Noncopyable {
  typedef ScopedLockImpl<Spinlock> Lock;

public:
  Spinlock() { pthread_spin_init(&m_lock, 0); }
  ~Spinlock() { pthread_spin_destroy(&m_lock); }
  void lock() { pthread_spin_lock(&m_lock); }
  void unlock() { pthread_spin_unlock(&m_lock); }

private:
  pthread_spinlock_t m_lock;
};

class CASLock : Noncopyable {
public:
  CASLock() { m_mutex.clear(); }
  ~CASLock() {}
  void lock() {
    while (std::atomic_flag_test_and_set_explicit(
        &m_mutex, std::memory_order::memory_order_acquire))
      ;
  }

  void unlock() {
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
  }

private:
  volatile std::atomic_flag m_mutex; // 表示每次都到内存中取数据
};

typedef Mutex MutexType;
} // namespace spadger

#endif