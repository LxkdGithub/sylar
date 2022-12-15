/*
 * @Author: lxk
 * @Date: 2022-10-14 17:34:32
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-16 11:45:57
 */
#include "iomanager.h"
#include "spadger.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

void test_fiber() {
  SPADGER_LOG_INFO(g_logger) << "test fiber";

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  //   addr.sin_addr.s_addr = inet_addr("180.101.49.11");
  inet_pton(AF_INET, "180.101.49.11", &addr.sin_addr.s_addr);

  int rt = connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr));
  if (rt != 0) {
    if (errno == EINPROGRESS) {
      SPADGER_LOG_INFO(g_logger)
          << "add event errno=" << errno << " " << strerror(errno);
      spadger::IOManager::GetThis()->addEvent(
          sock, spadger::IOManager::READ,
          []() { SPADGER_LOG_INFO(g_logger) << "read callback"; });

      spadger::IOManager::GetThis()->addEvent(
          sock, spadger::IOManager::WRITE, [sock]() {
            SPADGER_LOG_INFO(g_logger) << "write callback";
            spadger::IOManager::GetThis()->cancelEvent(
                sock, spadger::IOManager::READ);
            close(sock);
          });
    } else {
      SPADGER_LOG_INFO(g_logger)
          << "else errno=" << errno << " " << strerror(errno);
    }
  }
}

void test1() {
  spadger::IOManager iom;
  iom.schedule(&test_fiber);
}

spadger::Timer::ptr timer;
void test_timer() {
  spadger::IOManager iom(2); // 两个线程
  timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        SPADGER_LOG_DEBUG(g_logger) << "hello i=" << i;
        if (++i == 5) {
          //   timer->cancel();
          timer->reset(2000, true);
        }
      },
      true);
}

int main() {
  spadger::Thread::SetName("main");
  //   test1();
  test_timer();
  return 0;
}