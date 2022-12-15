/*
 * @Author: lxk
 * @Date: 2021-12-07 21:33:30
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-23 10:50:28
 */
#include "util.h"
#include "fiber.h"
#include "log.h"
#include <dirent.h>
#include <execinfo.h>
#include <sstream>
#include <string.h>

namespace spadger {

static spadger::Logger::ptr g_logger = SPADGER_LOG_NAME("system");

pid_t GetThreadId() { return syscall(__NR_gettid); }

uint32_t GetFiberId() { return Fiber::GetFiberId(); }

void Backtrace(std::vector<std::string> &bt, int size, int skip) {
  // 尽可能将信息放在堆里 而不是栈里
  void **array = (void **)malloc(sizeof(void *) * size);
  size_t s = backtrace(array, size);

  // 把array存储的信息转化为char* 格式的 但是函数里分配的内存
  // 所以需要free(strings)
  char **strings = backtrace_symbols(array, s);
  if (strings == NULL) {
    SPADGER_LOG_ERROR(g_logger) << "BackTrace Error." << std::endl;
    return;
  }

  for (size_t i = skip; i < s; i++) {
    bt.push_back(strings[i]); // 使用char* 构造string(构造函数)
  }

  free(strings);
  free(array);
}

// 我马上要skip 因为BackTrace函数不属于业务代码范畴 所以把最近的去掉
std::string BacktraceToString(int size, int skip, const std::string &prefix) {
  std::vector<std::string> bt;
  // 将信息放在bt里
  Backtrace(bt, size, skip);
  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); ++i) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}
std::string Time2Str(time_t ts, const std::string format) {
  struct tm tm;
  localtime_r(&ts, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), format.c_str(), &tm);
  return buf;
}

time_t Str2Time(const char *str, const char *format) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  if (!strptime(str, format, &tm)) {
    return 0;
  }
  return mktime(&tm);
}

std::string ToUpper(const std::string &name) {
  std::string rt = name;
  std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
  return rt;
}

std::string ToLower(const std::string &name) {
  std::string rt = name;
  std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
  return rt;
}

void FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string &path, const std::string &subfix) {
  // access pat using readl UID and GID
  // TODO:
  if (access(path.c_str(), 0) != 0) {
    return;
  }
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }
  struct dirent *dp = nullptr;
  while ((dp = readdir(dir)) != nullptr) {
    if (dp->d_type == DT_DIR) {
      if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, ".")) {
        continue;
      }
      ListAllFile(files, path + "/" + dp->d_name, subfix);
    } else if (dp->d_type == DT_REG) { // regular file
      std::string filename(dp->d_name);
      if (subfix.empty()) {
        files.push_back(path + "/" + filename);
      } else {
        if (subfix.size() > filename.size()) {
          continue;
        }
        if (filename.substr(filename.size() - subfix.size()) == subfix) {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }
  closedir(dir);
}

bool FSUtil::Mkdir(const std::string &dirname) { return false; }
bool FSUtil::IsRunningPidfile(const std::string &pidfile) { return false; }
bool FSUtil::Rm(const std::string &path) { return false; }
bool FSUtil::Mv(const std::string &from, const std::string &to) {
  return false;
}
bool FSUtil::Realpath(const std::string &path, std::string &rpath) {
  return false;
}
bool FSUtil::Symlink(const std::string &frm, const std::string &to) {
  return false;
}
bool FSUtil::Unlink(const std::string &filename, bool exist) { return false; }
std::string FSUtil::Dirname(const std::string &filename) { return ""; }
std::string FSUtil::Basename(const std::string &filename) { return ""; }
bool FSUtil::OpenForRead(std::ifstream &ifs, const std::string &filename,
                         std::ios_base::openmode mode) {
  return false;
}
bool FSUtil::OpenForWrite(std::ofstream &ofs, const std::string &filename,
                          std::ios_base::openmode mode) {
  return false;
}

uint64_t getCurrentMS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t getCurrentUS() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

} // end namespace spadger