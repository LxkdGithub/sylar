/*
 * @Author: lxk
 * @Date: 2022-10-22 16:52:45
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 10:58:48
 */
#include "socket.h"
#include "fd_manager.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

static spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

namespace spadger {

Socket::ptr Socket::CreateTCP(spadger::Address::ptr address) {
  Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDP(spadger::Address::ptr address) {
  Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
  sock->newSock();
  sock->m_isConnected = true;
  return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
  Socket::ptr sock(new Socket(IPv4, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
  Socket::ptr sock(new Socket(IPv4, UDP, 0));
  sock->newSock();
  sock->m_isConnected = true;
  return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
  Socket::ptr sock(new Socket(IPv6, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
  Socket::ptr sock(new Socket(IPv6, UDP, 0));
  sock->newSock();
  sock->m_isConnected = true;
  return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket() {
  Socket::ptr sock(new Socket(UNIX, TCP, 0));
  return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket() {
  Socket::ptr sock(new Socket(UNIX, UDP, 0));
  return sock;
}

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1), m_family(family), m_type(type), m_protocol(protocol),
      m_isConnected(false) {}
Socket::~Socket() { close(); }

int64_t Socket::getSendTimeout() {
  // 使用fd_manager中的FdCtx管理timeout
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_SNDTIMEO);
  }
  return -1;
}
void Socket::setSendTimeout(uint64_t v) {
  // v is ms
  struct timeval tv {
    int(v / 1000), int((v % 1000) * 1000)
  };
  setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout() {
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
  if (ctx) {
    return ctx->getTimeout(SO_RCVTIMEO);
  }
  return -1;
}
void Socket::setRecvTimeout(int64_t v) {
  struct timeval tv {
    int(v / 1000), int((v % 1000) * 1000)
  };
  setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void *result, socklen_t *len) {
  int rt = getsockopt(m_sock, level, option, result, len);
  if (rt) {
    SPADGER_LOG_DEBUG(g_logger)
        << "getOption sock=" << m_sock << " level=" << level
        << " option=" << option << " errno=" << errno
        << " error=" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::setOption(int level, int option, const void *value,
                       socklen_t len) {
  int rt = setsockopt(m_sock, level, option, value, len);
  if (rt) {
    SPADGER_LOG_DEBUG(g_logger)
        << "setOption sock=" << m_sock << " level=" << level
        << " option=" << option << " errno=" << errno
        << " error=" << strerror(errno);
    return false;
  }
  return true;
}

// action

// 设置一些option
void Socket::initSock() {
  int val = 1;
  setOption(SOL_SOCKET, SO_REUSEADDR, val);
  if (m_type == SOCK_STREAM) {
    setOption(IPPROTO_TCP, TCP_NODELAY, val);
  }
}

/// @brief Create a new sock and initSock.
/// @return true if succeed.
bool Socket::newSock() {
  m_sock = socket(m_family, m_type, m_protocol);
  if (m_sock != -1) {
    initSock();
  } else {
    SPADGER_LOG_ERROR(g_logger)
        << "socket(" << m_family << ", " << m_type << ", " << m_protocol
        << ") errno=" << errno << " error=" << strerror(errno);
  }
  return true;
}

/// @brief Init Socket using a socket
/// @param socket
/// @return true if succeed.
bool Socket::init(int socket) {
  FdCtx::ptr ctx = FdMgr::GetInstance()->get(socket);
  if (ctx) {
    m_sock = socket;
    m_isConnected = true;
    initSock();
    getLocalAddress();
    getRemoteAddress();
    return true;
  }
  return false;
}

Socket::ptr Socket::accept() {
  // accept 会之后返回一个Socket对象
  Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
  int newsock = ::accept(m_sock, nullptr, nullptr);
  if (newsock == -1) {
    SPADGER_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno
                                << " error=" << strerror(errno);
    return nullptr;
  }
  if (sock->init(newsock)) {
    return sock;
  }
  return nullptr;
}

//========================  bind & connect   ================================

bool Socket::bind(const Address::ptr addr) {
  if (!isValid()) {
    newSock();
    if (!isValid()) {
      return false;
    }
  }
  if (addr->getFamily() != m_family) {
    SPADGER_LOG_ERROR(g_logger)
        << "bind sock.family(" << m_family << ") addr.family("
        << addr->getFamily() << ") not equal, addr=" << addr->toString();
    return false;
  }
  if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
    SPADGER_LOG_ERROR(g_logger)
        << "bind error errno=" << errno << " strerr=" << strerror(errno);
    return false;
  }
  // bind的是本地地址
  getLocalAddress();
  return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
  if (!isValid()) {
    newSock();
    if (!isValid()) {
      return false;
    }
  }

  if (addr->getFamily() != m_family) {
    SPADGER_LOG_ERROR(g_logger)
        << "connect sock.family(" << m_family << ") addr.family("
        << addr->getFamily() << ") not equal, addr=" << addr->toString();
    return false;
  }
  if (timeout_ms == (uint64_t)-1) {
    if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
      SPADGER_LOG_ERROR(g_logger)
          << "sock=" << m_sock << " connect" << addr->toString()
          << ") errno=" << errno << " error=" << strerror(errno);
      return false;
    }
  } else {
    // 处理超时 timeout_ms
    if (::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(),
                               timeout_ms)) {
      SPADGER_LOG_ERROR(g_logger)
          << "sock=" << m_sock << " connect" << addr->toString()
          << ") errno=" << errno << " error=" << strerror(errno);
      return false;
    }
  }
  m_isConnected = true;
  getRemoteAddress();
  getLocalAddress();
  return true;
}

bool Socket::listen(int backlog) {
  if (!isValid()) {
    SPADGER_LOG_ERROR(g_logger) << "listen: invalid error";
    return false;
  }
  if (::listen(m_sock, backlog) != 0) {
    SPADGER_LOG_ERROR(g_logger)
        << "listen error errno=" << errno << " errstr=" << strerror(errno);
    return false;
  }
  return true;
}

bool Socket::close() {
  if (!m_isConnected && m_sock == -1) {
    return true;
  }
  m_isConnected = false;
  if (m_sock != -1) {
    ::close(m_sock);
    m_sock = -1;
  }
  return true;
}

//========================  send & recv  ================================

int Socket::send(const void *buffer, size_t length, int flags) {
  if (isConnected()) {
    return ::send(m_sock, buffer, length, flags);
  }
  return -1;
}

// 干脆叫snedmsg得了
int Socket::send(const iovec *buffer, size_t length, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffer;
    msg.msg_iovlen = length;
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    return ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                    to->getAddrLen());
  }
  return -1;
}

// sendmsg + to怎么发送
int Socket::sendTo(const iovec *buffer, size_t length, const Address::ptr to,
                   int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffer;
    msg.msg_iovlen = length;
    msg.msg_name = to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) {
  if (isConnected()) {
    return ::recv(m_sock, buffer, length, flags);
  }
  return -1;
}

int Socket::recv(iovec *buffer, size_t length, int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffer;
    msg.msg_iovlen = length;
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

int Socket::recvFrom(void *buffer, size_t length, Address::ptr from,
                     int flags) {
  if (isConnected()) {
    socklen_t len = from->getAddrLen();
    return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
  }
  return -1;
}

int Socket::recvFrom(iovec *buffer, size_t length, Address::ptr from,
                     int flags) {
  if (isConnected()) {
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec *)buffer;
    msg.msg_iovlen = length;
    msg.msg_name = from->getAddr();
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg(m_sock, &msg, flags);
  }
  return -1;
}

// status
Address::ptr Socket::getRemoteAddress() {
  if (m_remoteAddress) {
    return m_remoteAddress;
  }

  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknownAddress(m_family));
  }
  socklen_t addrlen = result->getAddrLen();
  if (getpeername(m_sock, result->getAddr(), &addrlen)) {
    SPADGER_LOG_ERROR(g_logger)
        << "getpeername error sock=" << m_sock << " errno=" << errno
        << " error=" << strerror(errno);
    return Address::ptr(new UnknownAddress(m_family));
  }
  // 对于UnixAddres需要单独设置addrlen
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  m_remoteAddress = result;
  return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
  if (m_remoteAddress) {
    return m_remoteAddress;
  }

  Address::ptr result;
  switch (m_family) {
  case AF_INET:
    result.reset(new IPv4Address());
    break;
  case AF_INET6:
    result.reset(new IPv6Address());
    break;
  case AF_UNIX:
    result.reset(new UnixAddress());
    break;
  default:
    result.reset(new UnknownAddress(m_family));
  }
  socklen_t addrlen = result->getAddrLen();
  if (getpeername(m_sock, result->getAddr(), &addrlen)) {
    SPADGER_LOG_ERROR(g_logger)
        << "getpeername error sock=" << m_sock << " errno=" << errno
        << " error=" << strerror(errno);
    return Address::ptr(new UnknownAddress(m_family));
  }
  // 对于UnixAddres需要单独设置addrlen
  if (m_family == AF_UNIX) {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrLen(addrlen);
  }
  m_localAddress = result;
  return m_localAddress;
}

bool Socket::isValid() const { return m_sock != -1; }
int Socket::getError() {
  // getsockopt中有SO_ERROR可以获取
  int error = 0;
  socklen_t len = sizeof(error);
  if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
  os << "[Socket sock=" << m_sock << " isConnected=" << m_isConnected
     << " family=" << m_family << " type=" << m_type
     << " protocol=" << m_protocol;
  if (m_localAddress) {
    os << " localAddress=" << m_localAddress;
  }
  if (m_remoteAddress) {
    os << " remoteAddress=" << m_remoteAddress;
  }
  return os;
}

// action
bool Socket::cancelRead() {
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

bool Socket::cancelWrite() {
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
}

bool Socket::cancelAccept() {
  // accept的sock只有READ这一个event
  return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
}

bool Socket::cancelAll() { return IOManager::GetThis()->cancelAll(m_sock); }

} // namespace spadger