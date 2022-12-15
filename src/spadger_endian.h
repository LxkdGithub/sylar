/*
 * @Author: lxk
 * @Date: 2022-10-20 21:27:09
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-21 19:57:23
 */
#ifndef __SPADGER_ENDIAN_H__
#define __SPADGER_ENDIAN_H__

#include <byteswap.h>
#include <stdint.h>

#define SPADGER_LITTLE_ENDIAN 1
#define SPADGER_BIG_ENDIAN 2

namespace spadger {

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
  return (T)bswap_64((uint64_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
  return (T)bswap_32((uint32_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
  return (T)bswap_16((uint16_t)value);
}

// 在x86中直接定义了BYTE_ORDER 为 LITTLE_ENDIAN
#if BYTE_ORDER == BIG_ENDIAN
#define SPADGER_BYTE_ORDER SPADGER_BIG_ENDIAN
#else
#define SPADGER_BYTE_ORDER SPADGER_LITTLE_ENDIAN
#endif

#if SPADGER_BYTE_ORDER == SPADGER_BIG_ENDIAN
template <class T> T byteswapOnLittleEndian(T t) { return t; }

template <class T> T byteswapOnBigEndian(T t) { return byteswap(t); }
#else
template <class T> T byteswapOnLittleEndian(T t) { return byteswap(t); }

template <class T> T byteswapOnBigEndian(T t) { return t; }
#endif

} // namespace spadger

#endif