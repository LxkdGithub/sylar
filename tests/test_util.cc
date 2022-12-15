/*
 * @Author: lxk
 * @Date: 2021-12-25 16:17:25
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 16:21:22
 */
#include "log.h"
#include "util.h"

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

int main() {
  // Test Backtrace.
  //   std::string res = spadger::BacktraceToString(10, 0, "--> ");
  //   SPADGER_LOG_INFO(g_logger) << "\n" << res;

  // Test SPADGER_ASSERT.
  SPADGER_ASSERT(false);
  return 0;
}