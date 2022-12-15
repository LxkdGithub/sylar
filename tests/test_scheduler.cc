/*
 * @Author: lxk
 * @Date: 2021-12-28 14:15:08
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-13 17:32:52
 */

#include "spadger.h"

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

static spadger::Mutex m_mutex;

void test_fiber() {
  static int s_count = 5;
  {
    spadger::Mutex::Lock lock(m_mutex);
    SPADGER_LOG_INFO(g_logger) << "test in fiber s_count=" << --s_count;
    // SPADGER_LOG_INFO(g_logger) << spadger::GetThreadId();
    // spadger::Scheduler::GetThis()->schedule(&test_fiber,
    // spadger::GetThreadId());    // 测试指定线程
  }
  sleep(1);
}

int main() {
  SPADGER_LOG_INFO(g_logger) << "main";
  spadger::Scheduler sc(3, false, "test");
  sc.start();
  // sleep(2);
  SPADGER_LOG_INFO(g_logger) << "main schedule";
  for (int i = 0; i < 5; i++) {
    sc.schedule(&test_fiber);
  }
  sc.stop();
  SPADGER_LOG_INFO(g_logger) << "over";
  return 0;
}