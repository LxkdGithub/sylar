/*
 * @Author: lxk
 * @Date: 2022-10-22 16:20:39
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 10:46:15
 */
#ifndef __SPADGER_SOCKET_H__
#define __SPADGER_SOCKET_H__

#include "address.h"
#include "noncopyable.h"
#include <memory>

namespace spadger {
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
  typedef std::shared_ptr<Socket> ptr;
  typedef std::weak_ptr<Socket> weak_ptr;

  enum Type {
    /// TCP类型
    TCP = SOCK_STREAM,
    /// UDP类型
    UDP = SOCK_DGRAM
  };

  /**
   * @brief Socket协议簇
   */
  enum Family {
    /// IPv4 socket
    IPv4 = AF_INET,
    /// IPv6 socket
    IPv6 = AF_INET6,
    /// Unix socket
    UNIX = AF_UNIX,
  };

  /**
   * @brief 创建TCP Socket(满足地址类型)
   * @param[in] address 地址
   */
  static Socket::ptr CreateTCP(spadger::Address::ptr address);

  /**
   * @brief 创建UDP Socket(满足地址类型)
   * @param[in] address 地址
   */
  static Socket::ptr CreateUDP(spadger::Address::ptr address);

  /**
   * @brief 创建IPv4的TCP Socket
   */
  static Socket::ptr CreateTCPSocket();

  /**
   * @brief 创建IPv4的UDP Socket
   */
  static Socket::ptr CreateUDPSocket();

  /**
   * @brief 创建IPv6的TCP Socket
   */
  static Socket::ptr CreateTCPSocket6();

  /**
   * @brief 创建IPv6的UDP Socket
   */
  static Socket::ptr CreateUDPSocket6();

  /**
   * @brief 创建Unix的TCP Socket
   */
  static Socket::ptr CreateUnixTCPSocket();

  /**
   * @brief 创建Unix的UDP Socket
   */
  static Socket::ptr CreateUnixUDPSocket();

  Socket(int family, int type, int protocol = 0);
  ~Socket();

  int64_t getSendTimeout();
  void setSendTimeout(uint64_t v);

  int64_t getRecvTimeout();
  void setRecvTimeout(int64_t v);

  bool getOption(int level, int option, void *result, socklen_t *len);
  template <typename T> bool getOption(int level, int option, T &result) {
    socklen_t length = sizeof(T);
    return getOption(level, option, &result, length);
  }
  bool setOption(int level, int option, const void *value, socklen_t len);
  template <typename T> bool setOption(int level, int option, const T &value) {
    socklen_t length = sizeof(T);
    return setOption(level, option, &value, length);
  }

  // action
  bool init(int socket); // 使用一个socketfd初始化
  Socket::ptr accept();
  bool bind(const Address::ptr addr);
  bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
  bool listen(int backlog = SOMAXCONN);
  bool close();

  int send(const void *buffer, size_t length, int flags = 0);
  int send(const iovec *buffer, size_t length, int flags = 0);
  int sendTo(const void *buffer, size_t length, const Address::ptr to,
             int flags = 0);
  int sendTo(const iovec *buffer, size_t length, const Address::ptr to,
             int flags = 0);

  int recv(void *buffer, size_t length, int flags = 0);
  int recv(iovec *buffer, size_t length, int flags = 0);
  int recvFrom(void *buffer, size_t length, Address::ptr from, int flags = 0);
  int recvFrom(iovec *buffer, size_t length, Address::ptr from, int flags = 0);

  // status
  Address::ptr getRemoteAddress();
  Address::ptr getLocalAddress();

  int getFamily() { return m_family; }
  int getType() { return m_type; }
  int getProtocol() { return m_protocol; }
  bool isConnected() { return m_isConnected; }
  bool isValid() const;
  int getError();

  std::ostream &dump(std::ostream &os) const;
  int getSocket() { return m_sock; }

  // action
  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

private:
  void initSock();
  bool newSock();

private:
  int m_sock;
  int m_family;
  int m_type;
  int m_protocol;
  int m_isConnected;

  Address::ptr m_localAddress;
  Address::ptr m_remoteAddress;
};

} // namespace spadger

#endif
