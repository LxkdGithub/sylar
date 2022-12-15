/*
 * @Author: lxk
 * @Date: 2022-10-20 20:12:03
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-22 19:31:11
 */
#ifndef __SPADGER_ADDRESS_H__
#define __SPADGER_ADDRESS_H__

#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

namespace spadger {

class IPAddress;

class Address {
public:
  typedef std::shared_ptr<Address> ptr;
  virtual ~Address() {}

  /**
   * @brief 通过sockaddr指针创建Address
   * @param[in] addr sockaddr指针
   * @param[in] addrlen sockaddr的长度
   * @return 返回和sockaddr相匹配的Address,失败返回nullptr
   */
  static Address::ptr Create(const sockaddr *addr, socklen_t addrlen);

  /**
   * @brief 通过host地址返回对应条件的所有Address
   * @param[out] result 保存满足条件的Address
   * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
   * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
   * @return 返回是否转换成功
   */
  static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family = AF_INET, int type = 0, int protocol = 0);
  /**
   * @brief 通过host地址返回对应条件的任意Address
   * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
   * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
   * @return 返回满足条件的任意Address,失败返回nullptr
   */
  static Address::ptr LookupAny(const std::string &host, int family = AF_INET,
                                int type = 0, int protocol = 0);
  /**
   * @brief 通过host地址返回对应条件的任意IPAddress
   * @param[in] host 域名,服务器名等.举例: www.sylar.top[:80] (方括号为可选内容)
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @param[in] type socketl类型SOCK_STREAM、SOCK_DGRAM 等
   * @param[in] protocol 协议,IPPROTO_TCP、IPPROTO_UDP 等
   * @return 返回满足条件的任意IPAddress,失败返回nullptr
   */
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string &host,
                                                       int family = AF_INET,
                                                       int type = 0,
                                                       int protocol = 0);

  /**
   * @brief 返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
   * @param[out] result 保存本机所有地址
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @return 是否获取成功
   */
  static bool GetInterfaceAddresses(
      std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
      int family = AF_INET);
  /**
   * @brief 获取指定网卡的地址和子网掩码位数
   * @param[out] result 保存指定网卡所有地址
   * @param[in] iface 网卡名称
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @return 是否获取成功
   */
  static bool
  GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                        const std::string &iface, int family = AF_INET);

  int getFamily() const;
  virtual sockaddr *getAddr() = 0;
  virtual const sockaddr *getAddr() const = 0;
  virtual socklen_t getAddrLen() const = 0;
  virtual std::ostream &insert(std::ostream &os) const = 0;

  std::string toString() const;
  bool operator<(const Address &rhs) const;
  bool operator==(const Address &rhs) const;
  bool operator!=(const Address &rhs) const;
};

// ============================  IP Address ================================

class IPAddress : public Address {
public:
  typedef std::shared_ptr<IPAddress> ptr;

  static IPAddress::ptr Create(const char *address, uint16_t port = 0);

  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  virtual uint32_t getPort() const = 0;
  virtual void setPort(uint16_t port) = 0;
};

class IPv4Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv4Address> ptr;

  // 直接使用sockaddr_in 创建
  IPv4Address(const sockaddr_in &addr);
  // 使用uint32_t的addr
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
  // 使用地址字符串192.168.1.1
  static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

  // Address Interface
  sockaddr *getAddr() override;
  const sockaddr *getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &) const override;

  // IPAddress Interface
  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  uint32_t getPort() const override;
  void setPort(uint16_t port) override;

private:
  sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
  typedef std::shared_ptr<IPv6Address> ptr;

  IPv6Address();
  IPv6Address(const sockaddr_in6 &addr);
  IPv6Address(const uint8_t address[16], uint16_t port = 0);
  static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

  sockaddr *getAddr() override;
  const sockaddr *getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networkAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  uint32_t getPort() const override;
  void setPort(uint16_t port) override;

private:
  sockaddr_in6 m_addr;
};

// ============================  Unix Address ================================

class UnixAddress : public Address {
public:
  typedef std::shared_ptr<UnixAddress> ptr;

  UnixAddress();
  // use unix sock, so we need a socket path.
  UnixAddress(const std::string &path);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  void setAddrLen(uint32_t v);
  std::ostream &insert(std::ostream &os) const override;
  std::string getPath() const;

private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

class UnknownAddress : public Address {
public:
  typedef std::shared_ptr<UnknownAddress> ptr;

  UnknownAddress(int family);
  UnknownAddress(const sockaddr &addr);

  // Address Interface
  sockaddr *getAddr() override;
  const sockaddr *getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

private:
  sockaddr m_addr;
};

std::ostream &operator<<(std::ostream &os, const Address &addr);

} // namespace spadger

#endif