/*
 * @Author: lxk
 * @Date: 2022-10-16 11:55:14
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-22 20:59:24
 */
#ifndef __SPADGER_HOOK_H__
#define __SPADGER_HOOK_H__
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace spadger {
bool is_hook_enable();
void set_hook_enable(bool flag);
} // namespace spadger

extern "C" {

// ========================  sleep 相关 ===========================
typedef unsigned int (*sleep_fun)(unsigned int seconds);
// using sleep_func = unisgned int (*)(unsigned int);
// 通过设置sleep_f 就可以达到使用自己和使用系统的选择问题
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t seconds);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

// ========================  socket 相关 ===========================

typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr,
                           socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen);
extern accept_fun accept_f;

typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                                struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len,
                              int flags, const struct sockaddr *dest_addr,
                              socklen_t addrlen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval,
                              socklen_t *optlen);
extern getsockopt_fun getsockopt_f;
typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                              const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd, const struct sockaddr *addr,
                                socklen_t addrlen, uint64_t timeout_ms);
}

#endif