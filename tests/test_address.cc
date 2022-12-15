/*
 * @Author: lxk
 * @Date: 2022-10-22 15:40:14
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-22 16:15:57
 */
#include "address.h"
#include "log.h"
#include <map>
#include <vector>

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

void test() {
  std::vector<spadger::Address::ptr> addrs;
  bool v = spadger::Address::Lookup(addrs, "www.baidu.com:ftp");
  if (!v) {
    SPADGER_LOG_ERROR(g_logger) << "lookup failed!";
  }
  for (size_t i = 0; i < addrs.size(); i++) {
    SPADGER_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
  }
}

void test_iface() {
  std::multimap<std::string, std::pair<spadger::Address::ptr, uint32_t>>
      results;
  bool v = spadger::Address::GetInterfaceAddresses(results);
  if (!v) {
    SPADGER_LOG_ERROR(g_logger) << "test_iface failed";
  }
  for (auto &i : results) {
    SPADGER_LOG_INFO(g_logger) << i.first << "  " << i.second.first->toString()
                               << ":" << i.second.second;
  }
}

void ipv4_test() {
  spadger::IPAddress::ptr addr = spadger::IPAddress::Create("www.baidu.com");
  SPADGER_LOG_INFO(g_logger) << addr->toString();
}

int main() {
  //   test();
  //   test_iface();
  ipv4_test();
}