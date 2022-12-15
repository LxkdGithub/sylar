/*
 * @Author: lxk
 * @Date: 2021-12-17 20:15:12
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 15:17:42
 */
#include "src/spadger.h"
#include <unistd.h>
#include <vector>

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();
spadger::Mutex m_mutex;

int count = 0;
void func1() {
  SPADGER_LOG_INFO(g_logger)
      << "name:" << spadger::Thread::GetName()
      << " this.name:" << spadger::Thread::GetThis()->GetName()
      << " id:" << spadger ::GetThreadId()
      << " this.id:" << spadger::Thread::GetThis()->getTid() << std::endl;
}

void func2() {
  while (true) {
    SPADGER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  }
}

int main() {
  SPADGER_LOG_INFO(g_logger) << "thread test begin";
  g_logger->clearAppenders();
  g_logger->addAppender(
      spadger::LogAppender::ptr(new spadger::FileLogAppender("log.txt")));
  std::vector<spadger::Thread::ptr> thrs;
  for (int i = 0; i < 1; i++) {
    spadger::Thread::ptr thr(
        new spadger::Thread(&func2, "name_" + std::to_string(i)));
    thrs.push_back(thr);
  }
  for (size_t i = 0; i < thrs.size(); i++) {
    thrs[i]->join();
  }
  SPADGER_LOG_INFO(g_logger) << "thread test end";
}