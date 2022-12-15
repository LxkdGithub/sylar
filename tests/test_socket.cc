/*
 * @Author: lxk
 * @Date: 2022-10-23 10:47:01
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 11:00:25
 */
#include "iomanager.h"
#include "socket.h"
#include "spadger.h"
#include "util.h"

static spadger::Logger::ptr g_looger = SPADGER_LOG_ROOT();

void test_socket() {
  // std::vector<spadger::Address::ptr> addrs;
  // spadger::Address::Lookup(addrs, "www.baidu.com", AF_INET);
  // spadger::IPAddress::ptr addr;
  // for(auto& i : addrs) {
  //    SPADGER_LOG_INFO(g_looger) << i->toString();
  //    addr = std::dynamic_pointer_cast<spadger::IPAddress>(i);
  //    if(addr) {
  //        break;
  //    }
  //}
  spadger::IPAddress::ptr addr =
      spadger::Address::LookupAnyIPAddress("www.baidu.com");
  if (addr) {
    SPADGER_LOG_INFO(g_looger) << "get address: " << addr->toString();
  } else {
    SPADGER_LOG_ERROR(g_looger) << "get address fail";
    return;
  }

  spadger::Socket::ptr sock = spadger::Socket::CreateTCP(addr);
  addr->setPort(80);
  SPADGER_LOG_INFO(g_looger) << "addr=" << addr->toString();
  if (!sock->connect(addr)) {
    SPADGER_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
    return;
  } else {
    SPADGER_LOG_INFO(g_looger)
        << "connect " << addr->toString() << " connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\r\n";
  int rt = sock->send(buff, sizeof(buff));
  if (rt <= 0) {
    SPADGER_LOG_INFO(g_looger) << "send fail rt=" << rt;
    return;
  }

  std::string buffs;
  buffs.resize(4096);
  rt = sock->recv(&buffs[0], buffs.size());

  if (rt <= 0) {
    SPADGER_LOG_INFO(g_looger) << "recv fail rt=" << rt;
    return;
  }

  buffs.resize(rt);
  SPADGER_LOG_INFO(g_looger) << buffs;
}

void test2() {
  spadger::IPAddress::ptr addr =
      spadger::Address::LookupAnyIPAddress("www.baidu.com:80");
  if (addr) {
    SPADGER_LOG_INFO(g_looger) << "get address: " << addr->toString();
  } else {
    SPADGER_LOG_ERROR(g_looger) << "get address fail";
    return;
  }

  spadger::Socket::ptr sock = spadger::Socket::CreateTCP(addr);
  if (!sock->connect(addr)) {
    SPADGER_LOG_ERROR(g_looger) << "connect " << addr->toString() << " fail";
    return;
  } else {
    SPADGER_LOG_INFO(g_looger)
        << "connect " << addr->toString() << " connected";
  }

  uint64_t ts = spadger::getCurrentUS();
  for (size_t i = 0; i < 10000000000ul; ++i) {
    if (int err = sock->getError()) {
      SPADGER_LOG_INFO(g_looger)
          << "err=" << err << " errstr=" << strerror(err);
      break;
    }

    // struct tcp_info tcp_info;
    // if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
    //    SPADGER_LOG_INFO(g_looger) << "err";
    //    break;
    //}
    // if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
    //    SPADGER_LOG_INFO(g_looger)
    //            << " state=" << (int)tcp_info.tcpi_state;
    //    break;
    //}
    static int batch = 10000000;
    if (i && (i % batch) == 0) {
      uint64_t ts2 = spadger::getCurrentUS();
      SPADGER_LOG_INFO(g_looger)
          << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
      ts = ts2;
    }
  }
}

int main(int argc, char **argv) {
  spadger::IOManager iom;
  iom.schedule(&test_socket);
  //   iom.schedule(&test2);
  return 0;
}