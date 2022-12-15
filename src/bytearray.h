/*
 * @Author: lxk
 * @Date: 2022-10-23 11:17:02
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 17:45:54
 */
#ifndef __SPADGER_BYTEARRAY_H__
#define __SPADGER_BYTEARRAY_H__

#include <memory>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace spadger {

class ByteArray {
public:
  typedef std::shared_ptr<ByteArray> ptr;
  struct Node {
    Node(size_t s);
    Node();
    ~Node();

    char *ptr;
    size_t size;
    Node *next;
  };

  ByteArray(size_t base_size = 4096); // size for per Node
  ~ByteArray();

  // ====================  write ======================
  // 定长 integer
  void writeFint8(int8_t value);
  void writeFuint8(uint8_t value);
  void writeFint16(int16_t value);
  void writeFuint16(uint16_t value);
  void writeFint32(int32_t value);
  void writeFuint32(uint32_t value);
  void writeFint64(int64_t value);
  void writeFuint64(uint64_t value);

  // 变长 integer
  // 8bit和16bit没有必要
  void writeInt32(int32_t value);
  void writeUint32(uint32_t value);
  void writeInt64(int64_t value);
  void writeUint64(uint64_t value);

  // float & double
  void writeFloat(float value);
  void writeDouble(double value);

  // string 分别为16 32 64 varint
  void writeStringF16(const std::string &value);
  void writeStringF32(const std::string &value);
  void writeStringF64(const std::string &value);
  void writeStringVint(const std::string &value);          // 变长
  void writeStringWithoutLength(const std::string &value); // 没有长度

  // ====================  read ======================
  // 定长 integer
  int8_t readFint8();
  uint8_t readFuint8();
  int16_t readFint16();
  uint16_t readFuint16();
  int32_t readFint32();
  uint32_t readFuint32();
  int64_t readFint64();
  uint64_t readFuint64();

  // 变长integer
  int32_t readInt32();
  uint32_t readUint32();
  int64_t readInt64();
  uint64_t readUint64();

  // float & double
  float readFloat();
  double readDouble();

  // string
  std::string readStringF16();
  std::string readStringF32();
  std::string readStringF64();
  std::string readStringVint();

  // ====================  内部操作 ======================
  void clear();
  void write(const void *buf, size_t size);
  void read(void *buf, size_t size);
  void read(void *buf, size_t size, size_t position) const;
  bool writeToFile(const std::string &name) const;
  bool readFromFile(const std::string &name);

  size_t getPosition() const { return m_position; }
  void setPosition(size_t v);

  size_t getBaseSize() const { return m_baseSize; } // 只能读取 不能修改了
  size_t getReadSize() const { return m_size - m_position; } // 可读的大小

  bool isLittleEndian() const;
  void setIsLittleEndian(bool val);

  std::string toString() const;
  std::string toHexString() const;

  size_t getSize() const { return m_size; }
  /**
   * @brief 获取可读取的缓存,保存成iovec数组
   * @param[out] buffers 保存可读取数据的iovec数组
   * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len =
   * getReadSize()
   * @return 返回实际数据的长度
   */
  uint64_t getReadBuffers(std::vector<iovec> &buffers,
                          uint64_t len = ~0ull) const;

  /**
   * @brief 获取可读取的缓存,保存成iovec数组,从position位置开始
   * @param[out] buffers 保存可读取数据的iovec数组
   * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len =
   * getReadSize()
   * @param[in] position 读取数据的位置
   * @return 返回实际数据的长度
   */
  uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                          uint64_t position) const;

  /**
   * @brief 获取可写入的缓存,保存成iovec数组
   * @param[out] buffers 保存可写入的内存的iovec数组
   * @param[in] len 写入的长度
   * @return 返回实际的长度
   * @post 如果(m_position + len) > m_capacity 则
   * m_capacity扩容N个节点以容纳len长度
   */
  uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

private:
  void addCapacity(size_t size);
  size_t getCapacity() const { return m_capacity - m_position; };

private:
  size_t m_baseSize;
  size_t m_position;
  size_t m_capacity;
  size_t m_size;
  int8_t m_endian;
  Node *m_root;
  Node *m_cur;
};

} // namespace spadger

#endif