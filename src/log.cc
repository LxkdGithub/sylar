/*
 * @Author: lxk
 * @Date: 2021-12-06 20:38:40
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 15:29:28
 */
#include "log.h"
#include "config.h"
#include <functional>
#include <iostream>
#include <time.h>

namespace spadger {

// #################################################################
// ############   FormatItem #######################################
class MessageFormatItem : public LogFormat::FormatItem {
public:
  MessageFormatItem(const std::string &str = "") {}
  ~MessageFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormat::FormatItem {
public:
  LevelFormatItem(const std::string &str = "") {}
  ~LevelFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << LogLevel::ToString(level);
  }
};

class ElapseFormatItem : public LogFormat::FormatItem {
public:
  ElapseFormatItem(const std::string &str = "") {}
  ~ElapseFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class ThreadIdFormatItem : public LogFormat::FormatItem {
public:
  ThreadIdFormatItem(const std::string &str = "") {}
  ~ThreadIdFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class ThreadNameFormatItem : public LogFormat::FormatItem {
public:
  ThreadNameFormatItem(const std::string &str = "") {}
  ~ThreadNameFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getThreadName();
  }
};

class FiberIdFormatItem : public LogFormat::FormatItem {
public:
  FiberIdFormatItem(const std::string &str = "") {}
  ~FiberIdFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class NameFormatItem : public LogFormat::FormatItem {
public:
  NameFormatItem(const std::string &str = "") {}
  ~NameFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getLogger()->getName();
  }
};

class TabFormatItem : public LogFormat::FormatItem {
public:
  TabFormatItem(const std::string &str) {}
  ~TabFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << "\t";
  }
};

class DateTimeFormatItem : public LogFormat::FormatItem {
public:
  DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
      : m_format(format) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }
  ~DateTimeFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->getTime();
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormat::FormatItem {
public:
  FilenameFormatItem(const std::string &str = "") {}
  ~FilenameFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormat::FormatItem {
public:
  LineFormatItem(const std::string &str = "") {}
  ~LineFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormat::FormatItem {
public:
  NewLineFormatItem(const std::string &str = "") {}
  ~NewLineFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormat::FormatItem {
public:
  StringFormatItem(const std::string str) : m_string(str) {}
  ~StringFormatItem() {}
  void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << m_string;
  }

private:
  std::string m_string;
};

// ############################################
// ######  LogLevel IMPL #####################
const char *LogLevel::ToString(LogLevel::Level level) {
  switch (level) {
#define XX(name)                                                               \
  case LogLevel::name:                                                         \
    return #name;                                                              \
    break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default:
    return "UNKNOW";
  }
  return "NNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(v, level)                                                           \
  if (#v == str) {                                                             \
    return LogLevel::level;                                                    \
  }
  XX(debug, DEBUG);
  XX(info, INFO);
  XX(warn, WARN);
  XX(error, ERROR);
  XX(fatal, FATAL);

  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
  return LogLevel::UNKNOW;
#undef XX
}

// ############################################
// ######  LogEvent IMPL #####################
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   const char *file, int32_t line, uint32_t elapse,
                   uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                   const std::string &thread_name)
    : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id),
      m_fiberId(fiber_id), m_time(time), m_threadName(thread_name),
      m_logger(logger), m_level(level) {}

LogEventWrap::~LogEventWrap() {
  m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream &LogEventWrap::getSS() { return m_event->getSS(); }

LogEvent::ptr LogEventWrap::getEvent() { return m_event; }

// #######################################################
// ######  Logger IMPL ###################################

Logger::Logger(const std::string &name)
    : m_name(name), m_level(LogLevel::DEBUG) {
  m_formatter.reset(new LogFormat(
      "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= getLevel()) {
    auto self = shared_from_this();
    Mutex::Lock lock(m_mutex);
    if (!m_appenders.empty()) {
      for (auto i : m_appenders) {
        i->log(self, level, event);
      }
    } else if (m_root) {
      m_root->log(level, event);
    }
  }
}

void Logger::debug(LogLevel::Level level, LogEvent::ptr event) {
  log(LogLevel::DEBUG, event);
}

void Logger::info(LogLevel::Level level, LogEvent::ptr event) {
  log(LogLevel::INFO, event);
}

void Logger::warn(LogLevel::Level level, LogEvent::ptr event) {
  log(LogLevel::WARN, event);
}

void Logger::error(LogLevel::Level level, LogEvent::ptr event) {
  log(LogLevel::ERROR, event);
}

void Logger::fatal(LogLevel::Level level, LogEvent::ptr event) {
  log(LogLevel::FATAL, event);
}

std::string Logger::toYamlString() {
  Mutex::Lock lock(m_mutex);
  YAML::Node node;
  node["name"] = m_name;
  if (m_level != LogLevel::UNKNOW)
    node["level"] = LogLevel::ToString(m_level);
  if (m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }

  for (auto &i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

// ===============  operation of appenders ================
void Logger::addAppender(LogAppender::ptr appender) {
  Mutex::Lock lock(m_mutex);
  if (!appender->m_hasFormatter) {
    appender->setFormatter(m_formatter, true);
  }
  m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  Mutex::Lock lock(m_mutex);
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
      break;
    }
  }
}

void Logger::clearAppenders() {
  Mutex::Lock lock(m_mutex);
  m_appenders.clear();
}

// ===============  operation of formatters ================

// set nad get of formatters.
void Logger::setFormatter(const std::string &val) {
  LogFormat::ptr new_val(new LogFormat(val));
  if (new_val->isError()) {
    std::cout << "Logger name=" << m_name << " setFormatter val=" << val
              << " error because of invalid formatter" << std::endl;
    return;
  }
  setFormatter(new_val);
}
void Logger::setFormatter(LogFormat::ptr val) {
  Mutex::Lock lock(m_mutex);
  m_formatter = val;
  for (auto &i : m_appenders) {
    if (!i->m_hasFormatter) {
      i->setFormatter(val, true); // lock in LogFormat class.
    }
  }
}
LogFormat::ptr Logger::getFormatter() {
  Mutex::Lock lock(m_mutex);
  return m_formatter;
}

//#####################################################################
//##############     LoggerManager class   ############################

LoggerManager::LoggerManager() {
  m_root.reset(
      new Logger("root")); // 不写root竟然是对的 很无语(明明必须要参数的)
  m_loggers[m_root->m_name] = m_root;

  LogAppender::ptr appender(new StdoutLogAppender);
  m_root->addAppender(appender);

  init();
}

void LoggerManager::init() {
  // TODO
}

Logger::ptr LoggerManager::getLogger(const std::string &name) {
  Mutex::Lock lock(m_mutex);
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }
  // create
  //   SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << "create logger:" << name;
  Logger::ptr logger(new Logger(name));
  logger->m_root = m_root; // all logger point to the m_root logger
  m_loggers.insert({name, logger});
  return logger;
}

std::string LoggerManager::toYamlString() {
  Mutex::Lock lock(m_mutex);
  YAML::Node node;
  for (auto &i : m_loggers) {
    node.push_back(YAML::Load(i.second->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

// ############################################
// ######  LogAppender IMPL #####################

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    Mutex::Lock lock(m_mutex);
    std::cout << m_formatter->format(logger, level, event);
  }
}

std::string StdoutLogAppender::toYamlString() {
  Mutex::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

// =================  FileLogAppender  ========================
FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
  reopen(); // init -> open filestream
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
  if (level >= m_level) {
    uint64_t now = time(0);
    if (now >= m_lastTime + 3) {
      reopen();
      m_lastTime = now;
    }
    Mutex::Lock lock(m_mutex);
    m_filestream << m_formatter->format(logger, level, event);
  }
}

bool FileLogAppender::reopen() {
  Mutex::Lock lock(m_mutex);
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);
  return !!m_filestream;
}

std::string FileLogAppender::toYamlString() {
  Mutex::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }

  std::stringstream ss;
  ss << node;
  return ss.str();
}

// ############################################
// ######  LogFormat IMPL #####################
LogFormat::LogFormat(const std::string &pattern) : m_pattern(pattern) {
  init();
}

void LogFormat::init() {
  // format parse more code...
  // str format type(0/1)
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string nstr;
  for (size_t i = 0; i < m_pattern.size(); ++i) {
    if (m_pattern[i] != '%') {
      nstr.append(1, m_pattern[i]);
      continue;
    }

    if ((i + 1) < m_pattern.size()) {
      if (m_pattern[i + 1] == '%') {
        nstr.append(1, '%');
        continue;
      }
    }

    size_t n = i + 1;
    int fmt_status = 0;
    size_t fmt_begin = 0;

    std::string str; // 存储模式 比如%d的d
    std::string fmt; // 存储一般的字符串 比如Level:%d的Level:这些
    while (n < m_pattern.size()) {
      // 此处匹配完毕(也就是遇到了新的%) 打算退出while
      if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                          m_pattern[n] != '}')) {
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }

      // 以下是需要匹配{}内的内容才需要的
      if (fmt_status == 0) {
        if (m_pattern[n] == '{') {
          str = m_pattern.substr(i + 1, n - i - 1);
          fmt_status = 1;
          fmt_begin = n;
          ++n;
          continue;
        }
      } else if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0; // 此段匹配结束了 begin位置还有用
          ++n;
          break;
        }
      }

      ++n;
      if (n == m_pattern.size()) {
        if (str.empty()) {
          str = m_pattern.substr(i + 1);
        }
      }
    }

    // 不管有没有匹配到{} 都会归0 如果正常的话
    if (fmt_status == 0) {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    } else if (fmt_status == 1) {
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      m_error = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }

  if (!nstr.empty()) {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }
  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string &str)>>
      s_format_items = {
#define XX(str, C)                                                             \
  {                                                                            \
#str, [](const std::string &str) { return FormatItem::ptr(new C(str)); }   \
  }

          XX(m, MessageFormatItem),  XX(p, LevelFormatItem),
          XX(r, ElapseFormatItem),   XX(c, NameFormatItem),
          XX(t, ThreadIdFormatItem), XX(N, ThreadNameFormatItem),
          XX(f, FilenameFormatItem), XX(F, FiberIdFormatItem),
          XX(n, NewLineFormatItem),  XX(d, DateTimeFormatItem),
          XX(l, LineFormatItem),     XX(T, TabFormatItem),
#undef XX
      };

  for (auto &i : vec) {
    if (std::get<2>(i) == 0) {
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      auto it = s_format_items.find(std::get<0>(i)); // 找到对应的callback函数
      if (it == s_format_items.end()) {
        m_items.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        m_error = true;
      } else {
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }
    // std::cout << "Format pattern init finished: " << m_items.size() << "
    // items." << std::endl;
  }

  // %m message body
  // %p -- level
  // %r -- time from start..
  // %c -- log name
  // %t -- thread Id
  // %n -- next line
  // %d -- time
  // %f -- filename
  // %l -- line number
}

std::string LogFormat::format(std::shared_ptr<Logger> logger,
                              LogLevel::Level level, LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &i : m_items) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

// ############################################
// ###### Struct Definition #####################

struct LogAppenderDefine {
  int type = 0; // 1 File 2.Stdout
  LogLevel::Level level = LogLevel::UNKNOW;
  std::string formatter;
  std::string file;
  bool operator==(const LogAppenderDefine &other) const {
    return type == other.type && level == other.level && file == other.file &&
           formatter == other.formatter;
  }
};

struct LogDefine {
  std::string name;
  LogLevel::Level level = LogLevel::UNKNOW;
  std::string formatter;
  std::vector<LogAppenderDefine> appenders;
  bool operator==(const LogDefine &other) const {
    return name == other.name && level == other.level &&
           formatter == other.formatter && appenders == other.appenders;
  }
  bool operator<(const LogDefine &other) const { return name < other.name; }
};

// 偏特化 用于和string转换
template <> class LexicalCast<std::string, LogDefine> {
public:
  LogDefine operator()(const std::string &v) {
    YAML::Node n = YAML::Load(v);
    LogDefine ld;
    if (!n["name"].IsDefined()) {
      std::cout << "log config error: name is null, " << n << std::endl;
      throw std::logic_error("log config name is null.");
    }
    ld.name = n["name"].as<std::string>();
    ld.level = LogLevel::FromString(
        n["level"].IsDefined() ? n["level"].as<std::string>() : "");
    if (n["formatter"].IsDefined()) {
      ld.formatter = n["formatter"].as<std::string>();
    }
    if (n["appenders"].IsDefined()) {
      for (size_t x = 0; x < n["appenders"].size(); x++) {
        auto a = n["appenders"][x];
        if (!a["type"].IsDefined()) {
          std::cout << "log config eror : appender type is null, " << a
                    << std::endl;
          continue;
        }
        std::string type = a["type"].as<std::string>();
        LogAppenderDefine lad;
        if (type == "FileLogAppender") {
          lad.type = 1;
          if (!a["file"].IsDefined()) {
            std::cout << "log config error: fileappender file is null, " << a
                      << std::endl;
            continue;
          }
          lad.file = a["file"].as<std::string>();
          if (a["formatter"].IsDefined()) {
            lad.formatter = a["formatter"].as<std::string>();
          }
        } else if (type == "StdoutLogAppender") {
          lad.type = 2;
          if (a["formatter"].IsDefined()) {
            lad.formatter = a["formatter"].as<std::string>();
          }
        } else {
          std::cout << "log config error: appender type is invalid, " << a
                    << std::endl;
          continue;
        }
        ld.appenders.push_back(lad);
      }
    }
    return ld;
  }
};

template <> class LexicalCast<LogDefine, std::string> {
public:
  std::string operator()(const LogDefine &i) {
    YAML::Node n;
    n["name"] = i.name;
    if (i.level != LogLevel::UNKNOW) {
      n["level"] = LogLevel::ToString(i.level);
    }
    if (!i.formatter.empty()) {
      n["formatter"] = i.formatter;
    }
    for (auto &a : i.appenders) {
      YAML::Node na;
      if (a.type == 1) {
        na["type"] = "FileAppender";
      } else if (a.type == 2) {
        na["type"] = "StdoutAppender";
      }
      if (a.level != LogLevel::UNKNOW) {
        na["level"] = LogLevel::ToString(a.level);
      }
      if (!a.formatter.empty()) {
        na["formatter"] = a.formatter;
      }
      n["appenders"].push_back(na);
    }
    std::stringstream ss;
    ss << n;
    return ss.str();
  }
};

ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    Config::Lookup("logs", std::set<LogDefine>(), "logs define");

// // 在main函数之前运行代码 就使用初始化变量的方式
struct LogIniter {
  LogIniter() {
    g_log_defines->addListener([](const std::set<LogDefine> &old_val,
                                  const std::set<LogDefine> &new_val) {
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << "on_logger_changed";
      // Add
      for (auto &i : new_val) {
        Logger::ptr logger;
        auto it = old_val.find(i);
        if (it == old_val.end()) {
          // add
          logger = SPADGER_LOG_NAME(i.name);
        } else {
          // modify
          if (*it == i) {
            logger = SPADGER_LOG_NAME(i.name); // 注意类型
          }
        }
        logger->setLevel(i.level);
        if (!i.formatter.empty()) {
          logger->setFormatter(i.formatter);
        }
        logger->clearAppenders();
        for (auto a : i.appenders) {
          LogAppender::ptr ap;
          if (a.type == 1) {
            ap.reset(new FileLogAppender(a.file));
          } else if (a.type == 2) {
            ap.reset(new StdoutLogAppender);
          }
          ap->setLevel(a.level);
          if (!a.formatter.empty()) {
            LogFormat::ptr fmt(new LogFormat(a.formatter));
            if (!fmt->isError()) {
              ap->setFormatter(fmt, false);
            } else {
              std::cout << "log.name=" << i.name << " appender type=" << a.type
                        << " formatter=" << a.formatter << " is invalid"
                        << std::endl;
            }
          }
          logger->addAppender(ap);
        }
      }
      // Del
      for (auto &i : old_val) {
        std::cout << i.name << std::endl;
        auto it = new_val.find(i);
        if (it == new_val.end()) {
          // del logger
          auto logger = SPADGER_LOG_NAME(i.name);
          logger->setLevel((LogLevel::Level)100);
          logger->clearAppenders();
        }
      }
    });
  }
};

static LogIniter __initer;

} // namespace spadger
