/*
 * @Author: lxk
 * @Date: 2021-12-07 21:27:09
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 10:53:55
 */
#ifndef __SPADGER_UTIL_H__
#define __SPADGER_UTIL_H__

#include "log.h"
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <string>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

// 在assert时先自己log输出 后assert throw error to upper layer.
#define SPADGER_ASSERT(x)                                                      \
  if (!(x)) {                                                                  \
    SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())                                      \
        << "ASSERTION: " #x << "\nbacktrace: \n"                               \
        << spadger::BacktraceToString(100, 2, "   ");                          \
    assert(x);                                                                 \
  }
#define SPADGER_ASSERT2(x, m)                                                  \
  if (!(x)) {                                                                  \
    SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())                                      \
        << "ASSERTION: " #x << "\n" #m << "\nbacktrace: \n"                    \
        << spadger::BacktraceToString(100, 2, "   ");                          \
    assert(x);                                                                 \
  }

namespace spadger {

/**
 * @description: Get the thread id in system(not the id in process)
 * @return ThreadId
 */
pid_t GetThreadId();

/**
 * @description: Get the FiberId
 * @return FiberId
 */
uint32_t GetFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
// size足够大 才可以放入所有信息
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string &prefix = "");

class FSUtil {
public:
  FSUtil();
  ~FSUtil();
  static void ListAllFile(std::vector<std::string> &files,
                          const std::string &path, const std::string &subfix);
  static bool Mkdir(const std::string &dirname);
  static bool IsRunningPidfile(const std::string &pidfile);
  static bool Rm(const std::string &path);
  static bool Mv(const std::string &from, const std::string &to);
  static bool Realpath(const std::string &path, std::string &rpath);
  static bool Symlink(const std::string &frm, const std::string &to);
  static bool Unlink(const std::string &filename, bool exist = false);
  static std::string Dirname(const std::string &filename);
  static std::string Basename(const std::string &filename);
  static bool OpenForRead(std::ifstream &ifs, const std::string &filename,
                          std::ios_base::openmode mode);
  static bool OpenForWrite(std::ofstream &ofs, const std::string &filename,
                           std::ios_base::openmode mode);
};

uint64_t getCurrentMS();

uint64_t getCurrentUS();

} // namespace spadger

#endif