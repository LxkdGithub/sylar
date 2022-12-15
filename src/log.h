#ifndef __SPADGER_LOG_H__
#define __SPADGER_LOG_H__

#include "singleton.h"
#include "thread.h"
#include "util.h"
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// 为了方便直接使用 而不需要反复创建logEvent 定义如下的宏
#define SPADGER_LOG_LEVEL(logger, level)                                       \
  if (logger->getLevel() <= level)                                             \
  spadger::LogEventWrap(                                                       \
      spadger::LogEvent::ptr(new spadger::LogEvent(                            \
          logger, level, __FILE__, __LINE__, 0, spadger::GetThreadId(),        \
          spadger::GetFiberId(), time(nullptr), spadger::Thread::GetName())))  \
      .getSS()

#define SPADGER_LOG_DEBUG(logger)                                              \
  SPADGER_LOG_LEVEL(logger, spadger::LogLevel::DEBUG)
#define SPADGER_LOG_INFO(logger)                                               \
  SPADGER_LOG_LEVEL(logger, spadger::LogLevel::INFO)
#define SPADGER_LOG_WARN(logger)                                               \
  SPADGER_LOG_LEVEL(logger, spadger::LogLevel::WARN)
#define SPADGER_LOG_ERROR(logger)                                              \
  SPADGER_LOG_LEVEL(logger, spadger::LogLevel::ERROR)
#define SPADGER_LOG_FATAL(logger)                                              \
  SPADGER_LOG_LEVEL(logger, spadger::LogLevel::FATAL)

#define SPADGER_LOG_ROOT() spadger::SingleLoggerMgr::GetInstance()->getRoot()

#define SPADGER_LOG_NAME(name)                                                 \
  spadger::SingleLoggerMgr::GetInstance()->getLogger(name)

#define B_LOG() SPADGER_LOG_INFO(SPADGER_LOG_ROOT())

namespace spadger {

// ########################################################
// ######  Logger Definition #########################
class Logger;
class LoggerManager;

// log level
class LogLevel {
public:
  enum Level {
    UNKNOW = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };

  static const char *ToString(LogLevel::Level level);
  static LogLevel::Level FromString(const std::string &str);
};

// #####################################################################
// ######  LogEvent Definition #######################################
class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, int32_t line, uint32_t elapse, uint32_t thread_id,
           uint32_t fiber_id, uint64_t time, const std::string &thread_name);

  const char *getFile() const { return m_file; }
  int32_t getLine() const { return m_line; }
  uint32_t getElapse() const { return m_elapse; }
  uint32_t getThreadId() const { return m_threadId; }
  std::string getThreadName() const { return m_threadName; }
  uint32_t getFiberId() const { return m_fiberId; }
  uint64_t getTime() const { return m_time; }
  std::string getContent() const { return m_ss.str(); }
  std::stringstream &getSS() { return m_ss; }
  std::shared_ptr<Logger> getLogger() { return m_logger; }
  LogLevel::Level getLevel() const { return m_level; }

private:
  /// 文件名
  const char *m_file = nullptr;
  /// 行号
  int32_t m_line = 0;
  /// 程序启动开始到现在的毫秒数
  uint32_t m_elapse = 0;
  /// 线程ID
  uint32_t m_threadId = 0;
  /// 协程ID
  uint32_t m_fiberId = 0;
  /// 时间戳
  uint64_t m_time = 0;
  /// 线程名称
  std::string m_threadName;
  /// 日志内容流
  std::stringstream m_ss;
  /// 日志器
  std::shared_ptr<Logger> m_logger;
  /// 日志等级
  LogLevel::Level m_level;
};

// 为什么需要LogEventWrap 是为了在释放LogEvent时进行log输出
// 但是又不好直接写在LogEvent的析构函数里 我们直接使用宏动态创建LogEvent
// 所以需要RAII机制
class LogEventWrap {
public:
  LogEventWrap(LogEvent::ptr event) : m_event(event) {}
  ~LogEventWrap();

  std::stringstream &getSS();

  LogEvent::ptr getEvent();

private:
  LogEvent::ptr m_event;
};

// ###############################################################
// ######  LogFormat Definition ###################################
class LogFormat {
public:
  typedef std::shared_ptr<LogFormat> ptr;
  LogFormat(const std::string &pattern);
  std::string getPattern() { return m_pattern; }
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);
  void init();
  bool isError() { return m_error; }

public: // public and others difference
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    FormatItem(const std::string &fmt = "") {}
    virtual ~FormatItem() {}
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };

private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
  bool m_error = false;
};

// #####################################################################
// ######  LogAppender Definition #######################################

class LogAppender {
  friend class Logger;

public:
  typedef std::shared_ptr<LogAppender> ptr;
  virtual ~LogAppender() {}

  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;
  virtual std::string toYamlString() = 0; // 留给子类实现
  //  -------- formatter --------------
  void setFormatter(LogFormat::ptr formatter, bool from_logger = false) {
    Mutex::Lock lock(m_mutex);
    m_formatter = formatter;
    if (!from_logger) {
      m_hasFormatter = true;
    }
  }
  LogFormat::ptr getFormatter() {
    Mutex::Lock lock(m_mutex);
    return m_formatter;
  }
  //  -------- level --------------
  LogLevel::Level getLevel() const { return m_level; }
  // 互斥的作用是构建临界代码区 表示两块代码不能够同时运行 setLevel可不兴这样啊
  void setLevel(LogLevel::Level val) { m_level = val; }

protected:
  LogLevel::Level m_level = LogLevel::DEBUG;
  bool m_hasFormatter = false;
  LogFormat::ptr m_formatter;
  spadger::Mutex m_mutex; // 修改formatter和log的时候需要加锁
};

// LogAppender Derived classes
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
           LogEvent::ptr event) override;
  std::string toYamlString() override;
};

class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);
  void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
           LogEvent::ptr event) override;
  std::string toYamlString() override;

  bool reopen();

private:
  std::string m_filename;
  std::ofstream m_filestream;
  uint64_t m_lastTime = 0; // 上次打开时间
};

// ##################################################################
// ######  Logger Definition ########################################
class Logger : public std::enable_shared_from_this<Logger> {
  friend class LoggerManager;

public:
  typedef std::shared_ptr<Logger> ptr;

  Logger(const std::string &name = "root");
  void log(LogLevel::Level level,
           LogEvent::ptr event); // call the logappender.log

  void debug(LogLevel::Level level, LogEvent::ptr event);
  void info(LogLevel::Level level, LogEvent::ptr event);
  void warn(LogLevel::Level level, LogEvent::ptr event);
  void error(LogLevel::Level level, LogEvent::ptr event);
  void fatal(LogLevel::Level level, LogEvent::ptr event);

  void setLevel(LogLevel::Level level) { m_level = level; }
  LogLevel::Level getLevel() const { return m_level; }

  const std::string &getName() const { return m_name; }

  // operation of appenders.
  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();

  // set nad get of formatters.
  void setFormatter(const std::string &val);
  void setFormatter(LogFormat::ptr val);
  LogFormat::ptr getFormatter();

  std::string toYamlString();

private:
  Mutex m_mutex;      // Should lock when modifing the logger's info.
  std::string m_name; // 默认不变
  LogLevel::Level m_level;
  LogFormat::ptr m_formatter;
  std::list<LogAppender::ptr> m_appenders;
  Logger::ptr m_root; // m_root point to the m_root logger.
};

// #####################################################################
// ######  LogManager Definition #######################################

/*
    日志器管理类(单例模式？？)
*/
class LoggerManager {
public:
  typedef std::shared_ptr<LoggerManager> ptr;
  LoggerManager();

  void init();
  Logger::ptr getRoot() const { return m_root; }
  Logger::ptr getLogger(const std::string &name);
  std::string toYamlString();

private:
  Mutex m_mutex;
  Logger::ptr m_root;
  std::map<std::string, Logger::ptr> m_loggers;
};

typedef spadger::Singleton<LoggerManager> SingleLoggerMgr;

} // namespace spadger

#endif
