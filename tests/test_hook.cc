/*
 * @Author: lxk
 * @Date: 2022-10-16 16:23:10
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-20 17:38:17
 */
#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

void test_sleep() {
  spadger::IOManager iom(1);
  iom.schedule([]() {
    sleep(2);
    SPADGER_LOG_INFO(g_logger) << "sleep 2";
  });

  iom.schedule([]() {
    sleep(3);
    SPADGER_LOG_INFO(g_logger) << "sleep 3";
  });
  SPADGER_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  inet_pton(AF_INET, "112.80.248.76", &addr.sin_addr.s_addr);

  SPADGER_LOG_INFO(g_logger) << "begin connect";
  int rt = connect(sock, (const sockaddr *)&addr, sizeof(addr));
  SPADGER_LOG_INFO(g_logger)
      << "connect finished! rt=" << rt << " errno=" << errno;

  if (rt) {
    return;
  }

  const char data[] = "GET / HTTP/1.0\r\n\r\n";
  rt = send(sock, data, sizeof(data), 0);
  SPADGER_LOG_INFO(g_logger)
      << "send finished! rt=" << rt << " errno=" << errno;

  if (rt <= 0) {
    return;
  }

  std::string buff;
  buff.resize(4096);

  rt = recv(sock, &buff[0], buff.size(), 0);
  SPADGER_LOG_INFO(g_logger)
      << "recv finished! rt=" << rt << " errno=" << errno;

  if (rt <= 0) {
    return;
  }

  buff.resize(rt);
  SPADGER_LOG_INFO(g_logger) << buff;
}

int main(int argc, char **argv) {
  // test_sleep();
  spadger::Thread::SetName("hello");
  spadger::IOManager iom(1);

  iom.schedule(test_sock);
  iom.stop();
  return 0;
}