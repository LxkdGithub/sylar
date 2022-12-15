/*
 * @Author: lxk
 * @Date: 2021-12-26 16:13:04
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 21:42:45
 */

#include "fiber.h"
#include "spadger.h"
#include <iostream>
spadger::Logger::ptr logger = SPADGER_LOG_ROOT();

void run_in_fiber() {
  SPADGER_LOG_INFO(logger) << "run in fiber111";
  spadger::Fiber::YieldToHold();
  SPADGER_LOG_INFO(logger) << "run in fiber222";
  // spadger::Fiber::YieldToHold();
}

void test_fiber() {
  {
    spadger::Fiber::GetThis(); // 构造main fiber
    SPADGER_LOG_INFO(logger) << "main func stage 1";
    spadger::Fiber::ptr fiber(new spadger::Fiber(run_in_fiber));
    fiber->swapIn();                                 // call 进入fiber
    SPADGER_LOG_INFO(logger) << "main func stage 2"; // 中途跳出
    fiber->swapIn();                                 // 再次进入
    SPADGER_LOG_INFO(logger) << "main func stage 3"; // 接触跳出
  }
  std::cout << "--------------------------------" << std::endl;
}

int main1() {
  spadger::Thread::SetName("main");
  test_fiber();
  return 0;
}

int main(int argc, char **argv) {
  spadger::Thread::SetName("main");

  std::vector<spadger::Thread::ptr> thrs;
  for (int i = 0; i < 3; i++) {
    thrs.push_back(spadger::Thread::ptr(
        new spadger::Thread(&test_fiber, "name" + std::to_string(i))));
  }
  for (auto i : thrs) {
    i->join();
  }
  return 0;
}
