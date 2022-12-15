/*
 * @Author: lxk
 * @Date: 2022-10-23 11:45:02
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 17:46:16
 */

#include "bytearray.h"
#include "log.h"
#include "spadger_endian.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string.h>

namespace spadger {

static spadger::Logger::ptr g_logger = SPADGER_LOG_ROOT();

ByteArray::Node::Node(size_t s) : ptr(new char[s]), size(s), next(nullptr) {}
ByteArray::Node::Node() : ptr(nullptr), size(0), next(nullptr) {}
ByteArray::Node::~Node() {
  if (ptr) {
    delete[] ptr;
  }
}

// =======================  ByteArray  =========================
ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size), m_position(0), m_capacity(base_size), m_size(0),
      m_endian(SPADGER_BIG_ENDIAN), m_root(new Node(base_size)), m_cur(m_root) {
}
// -----------------------    Write   -------------------------
bool ByteArray::isLittleEndian() const {
  return m_endian == SPADGER_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
  if (val) {
    m_endian = SPADGER_LITTLE_ENDIAN;
  } else {
    m_endian = SPADGER_BIG_ENDIAN;
  }
}

void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }

void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }

void ByteArray::writeFint16(int16_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
  if (m_endian != SPADGER_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

// -------------------------  变长 integer ------------------

static uint32_t EncodeZigzag32(const int32_t &v) {
  if (v < 0) {
    return ((uint32_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static uint64_t EncodeZigzag64(const int64_t &v) {
  if (v < 0) {
    return ((uint64_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static int32_t DecodeZigzag32(const uint32_t &v) { return (v >> 1) ^ -(v & 1); }

static int64_t DecodeZigzag64(const uint64_t &v) { return (v >> 1) ^ -(v & 1); }
// 1字节或者2字节是不需要变长的，没有必要 所以从4字节开始 read同理
void ByteArray::writeInt32(int32_t value) {
  // 先转变为unsigned
  writeUint32(EncodeZigzag32(value));
}
void ByteArray::writeUint32(uint32_t value) {
  uint8_t tmp[5]; // 32bit最多需要5个字节存放
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80; // 第一位为1 用于标记还有没有下一个字节
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i); // 写入
}

void ByteArray::writeInt64(int64_t value) {
  writeUint64(EncodeZigzag64(value));
}
void ByteArray::writeUint64(uint64_t value) {
  //和uint32同理
  uint8_t tmp[10]; // 32bit最多需要5个字节存放
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80; // 第一位为1 用于标记还有没有下一个字节
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i); // 写入
}

// -------------------------   float & double ---------------------
void ByteArray::writeFloat(float value) {
  // float是4字节，所以转化为32位的int进行写入
  uint32_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint32(v); // 固定32位的写入
}
void ByteArray::writeDouble(double value) {
  // 和double同理
  uint64_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint64(v); // 固定32位的写入
}

// -----------------   string 分别为16 32 64 varint -------------------------
// 这里的数值指的是长度对应的类型    -------------------------
void ByteArray::writeStringF16(const std::string &value) {
  // 因为是F16 所以需要自己标记长度
  writeFuint16(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringF32(const std::string &value) {
  writeFuint32(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringF64(const std::string &value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringVint(const std::string &value) {
  // 指的是存储的长度数值的大小是不定的
  // (比如长度比较短的字符串，长度如果存储为8字节就太长了)
  // 但是又无法确定使用哪个方法好，这个方法可以不用担心长度的大小浪费空间了
  writeUint64(value.size());
  write(value.c_str(), value.size());
}
void ByteArray::writeStringWithoutLength(const std::string &value) {
  write(value.c_str(), value.size());
}

// -----------------------    Read   -------------------------
// 定长 integer
int8_t ByteArray::readFint8() {
  int8_t v;
  read(&v, sizeof(v));
  return v;
}
uint8_t ByteArray::readFuint8() {
  uint8_t v;
  read(&v, sizeof(v));
  return v;
}

// 多个字节的读取需要考虑字节序了 就像write一样
// 因为代码相似 所以定义一个宏 (其实模板函数也可以的)
#define XX(type)                                                               \
  type v;                                                                      \
  read(&v, sizeof(v));                                                         \
  if (m_endian == SPADGER_BYTE_ORDER) {                                        \
    return v;                                                                  \
  } else {                                                                     \
    return byteswap(v);                                                        \
  }

int16_t ByteArray::readFint16() { XX(int16_t); }
uint16_t ByteArray::readFuint16() { XX(uint16_t); }
int32_t ByteArray::readFint32() { XX(int32_t); }
uint32_t ByteArray::readFuint32() { XX(uint32_t); }
int64_t ByteArray::readFint64() { XX(int64_t); }
uint64_t ByteArray::readFuint64() { XX(uint64_t); }
#undef XX

// -----------------------  变长integer --------------------------
int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }
uint32_t ByteArray::readUint32() {
  // writeUint32的逆过程
  // 通过每一个字节的首位bit是否为1判断是否黑暗又下一个字节可读
  // 每个字节都只有7bit有效位
  uint32_t result = 0;
  // 先读出的字节的地位 然后依次变高
  for (int i = 0; i < 32; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= (((uint32_t)b) << i);
      break;
    } else {
      result |= (((uint32_t)(b & 0x7f)) << i);
    }
  }
  return result;
}
int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }
uint64_t ByteArray::readUint64() {
  uint64_t result = 0;
  // 先读出的字节的地位 然后依次变高
  for (int i = 0; i < 64; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= (((uint32_t)b) << i);
      break;
    } else {
      result |= (((uint32_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

// ------------------------  float & double ------------------------
float ByteArray::readFloat() {
  // writeFloat的逆过程 先读出 再转化
  uint32_t v = readFuint32(); // 固定32位 这个可不行压缩的呀
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}
double ByteArray::readDouble() {
  uint64_t v = readFuint64(); // 固定32位 这个可不行压缩的呀
  double value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

// string
std::string ByteArray::readStringF16() {
  uint16_t len = readFuint16();
  std::string buf;
  buf.resize(len);
  // 虽然string.c_str()不可以写入 但是可以通过[]操作符取地址进行写入
  read(&buf[0], len);
  return buf;
}
std::string ByteArray::readStringF32() {
  uint32_t len = readFuint32();
  std::string buf;
  buf.resize(len);
  // 虽然string.c_str()不可以写入 但是可以通过[]操作符取地址进行写入
  read(&buf[0], len);
  return buf;
}
std::string ByteArray::readStringF64() {
  uint64_t len = readFuint64();
  std::string buf;
  buf.resize(len);
  // 虽然string.c_str()不可以写入 但是可以通过[]操作符取地址进行写入
  read(&buf[0], len);
  return buf;
}
std::string ByteArray::readStringVint() {
  uint64_t len = readUint64(); // 读取非定长的长度
  std::string buf;
  buf.resize(len);
  // 虽然string.c_str()不可以写入 但是可以通过[]操作符取地址进行写入
  read(&buf[0], len);
  return buf;
}

// ====================  内部操作 ======================
void ByteArray::clear() {
  m_position = 0;
  m_size = 0;
  m_capacity = m_baseSize;
  // 清空所有的Node 只留下一个m_root Node即可
  Node *tmp = m_root->next;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = nullptr;
}
void ByteArray::write(const void *buf, size_t size) {
  if (size == 0) {
    return;
  }
  addCapacity(size); // 确保有空间可用
  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  while (size > 0) {
    if (ncap >= size) {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_position += size;
      break;
    } else {
      // 当前Node放不下 先放慢当前Node，然后是下一个Node
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
      size -= ncap;
      m_position += ncap;
      bpos += ncap;
      // 开始写下一个Node
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
  if (m_position > m_size) {
    m_size = m_position;
  }
}
void ByteArray::read(void *buf, size_t size) {
  // TODO:
  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_position += size;
      break;
    } else {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
      size -= ncap;
      bpos += ncap;
      m_position += ncap;
      // 开始读下一个Node
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
}
void ByteArray::read(void *buf, size_t size, size_t position) const {
  // TODO:
  size_t npos = position % m_baseSize; // position有所不同罢了
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  Node *cur = m_cur;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, cur->ptr + npos, size);
      if (cur->size == (npos + size)) {
        cur = cur->next;
      }
      position += size;
      break;
    } else {
      memcpy((char *)buf + bpos, cur->ptr + npos, ncap);
      size -= ncap;
      bpos += ncap;
      position += ncap;
      // 开始读下一个Node
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
  }
}

void ByteArray::setPosition(size_t v) {
  if (v > m_capacity) {
    throw std::out_of_range("setPositino out of range");
  }
  m_position = v;
  if (m_position > m_size) {
    m_size = v;
  }
  // 从root处开始计算m_cur应该处于的位置
  m_cur = m_root;
  while (v >= m_cur->size) {
    v -= m_cur->size;
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string &name) const {
  // 为什么不一次读出来然后read(buf, size)呢？占内存呀
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    SPADGER_LOG_ERROR(g_logger)
        << "writeToFile name=" << name << " error , errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }

  int64_t read_size = getReadSize();
  int64_t pos = m_position;
  Node *cur = m_cur;

  while (read_size > 0) {
    int diff = pos % m_baseSize;
    // TODO: ? len的计算不太对吧?
    int64_t len =
        (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
    ofs.write(cur->ptr + diff, len);
    cur = cur->next;
    pos += len;
    read_size -= len;
  }

  return true;
}
bool ByteArray::readFromFile(const std::string &name) {
  // 为什么不一次读出来然后write(buf, size)呢？占内存呀
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);
  if (!ifs) {
    SPADGER_LOG_ERROR(g_logger)
        << "readFromFile name=" << name << " error, errno=" << errno
        << " errstr=" << strerror(errno);
    return false;
  }
  std::shared_ptr<char> buff(new char[m_baseSize],
                             [](char *ptr) { delete[] ptr; });
  while (!ifs.eof()) {
    ifs.read(buff.get(), m_baseSize); // pos可能不是在Node开始处 还有意义吗
    write(buff.get(), ifs.gcount());
  }
  return true;
}

void ByteArray::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  size_t old_cap = getCapacity();
  // 因为我们的capaciyt是以m_baseSize为单位分配的，所以可能会出现下面这种情况
  if (old_cap >= size) {
    return;
  }

  // 添加capacity是以m_baseSize为基本单位的 向上取整
  size = size - old_cap; // old_cap == k * m_baseSize
  size_t count = ceil(1.0 * size / m_baseSize);
  Node *tmp = m_root; //  m_root != nullptr
  while (tmp->next) {
    tmp = tmp->next;
  }

  Node *first = nullptr; // 新分配的第一个Node
  for (size_t i = 0; i < count; i++) {
    tmp->next = new Node(m_baseSize);
    if (first == nullptr) {
      tmp->next = first;
    }
    tmp = tmp->next;
    m_capacity += m_baseSize;
  }
  // 如果m_cur以前没有设置过
  if (old_cap == 0) {
    m_cur = first;
  }
}

std::string ByteArray::toString() const {
  std::string str;
  str.resize(getReadSize());
  if (str.empty()) {
    return str;
  }
  read(&str[0], str.size(), m_position);
  return str;
}
std::string ByteArray::toHexString() const {
  std::string str = toString();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i]
       << " ";
  }

  return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers,
                                   uint64_t len) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;

  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position % m_baseSize;
  size_t count = position / m_baseSize;
  Node *cur = m_root;
  while (count > 0) {
    cur = cur->next;
    --count;
  }

  size_t ncap = cur->size - npos;
  struct iovec iov;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }
  addCapacity(len);
  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

} // namespace spadger
