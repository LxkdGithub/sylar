/*
 * @Author: lxk
 * @Date: 2021-12-03 21:57:06
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 11:44:00
 */
#include "../src/config.h"
#include "../src/log.h"
#include <ctime>
#include <iostream>
#include <yaml-cpp/yaml.h>

// %m message body
// %p -- level
// %r -- time from start..
// %c -- log name
// %t -- thread Id
// %n -- next line
// %d -- time
// %f -- filename
// %l -- line number

int main() {
  // std::cout << "hello world!!!!!" << std::endl;
  // // Get Logger
  // spadger::Logger::ptr logger(new spadger::Logger("myLogger"));
  // logger->setLevel(spadger::LogLevel::DEBUG);
  // // Get Appender and Formatter
  // spadger::LogAppender::ptr appender(new spadger::StdoutLogAppender());
  // spadger::LogFormat::ptr formatter(new spadger::LogFormat(
  //     "[Time:%d] [logger:%c] [Level:%p] [Started:%r] [ThreadId:%t]
  //     [FileName:%f] [LineNum:%l] message:%m [%r] [%t] %n"));
  // appender->setFormatter(formatter);
  // // Add Formatter to Appender
  // logger->addAppender(appender);

  // // FileAppender
  // spadger::FileLogAppender::ptr fileAppender(new
  // spadger::FileLogAppender("log.txt")); if(fileAppender->reopen()) {
  //     std::cout << "yes" << std::endl;
  // } else {
  //     return 1;
  // }
  // fileAppender->setFormatter(formatter);
  // logger->addAppender(fileAppender);

  // // 旧版本的log输出 创建LogEvent的Level和log输出时的参数Level是一致的
  // 所以使用event->getLevel当做参数 spadger::LogEvent::ptr event(new
  // spadger::LogEvent(logger, spadger::LogLevel::INFO, __FILE__, __LINE__, 0,
  // spadger::GetThreadId(), spadger::GetFiberId(), time(0), "thread1"));
  // event->getSS() << "hello log succeed!";
  // event->getSS() << "world!! ";
  // logger->log(event->getLevel(), event);

  // // 新版本的输出 更为方便
  // SPADGER_LOG_ERROR(logger) << "qnmd";
  // SPADGER_LOG_LEVEL(logger, spadger::LogLevel::INFO) << "vdsvsdbsdb";

  // spadger::Config config;
  // std::string k1 = "k1";
  // spadger::ConfigVar<std::string>::ptr a = config.Lookup("dvs",
  // std::string("kdvs1"), "sdv"); std::cout << a->getValue() << std::endl;
  // a->addListener([](const std::string& lhs, const std::string& rhs) {
  //     std::cout << "lambda" << std::endl;
  // });
  // a->addListener([&](const std::string& lhs, const std::string& rhs) {
  //     SPADGER_LOG_INFO(logger) << "changed" << std::endl;
  // });

  // a->setValue("hello");
  // std::cout <<a->getValue() << std::endl;

  // //
  // #########################################################################
  // spadger::FileLogAppender::ptr f_appender1(new
  // spadger::FileLogAppender("log1.txt")); if(f_appender1->reopen()) {
  //     std::cout << "yes" << std::endl;
  // } else {
  //     return 1;
  // }
  // // SPADGER_LOG_INFO(spadger::LoggerMgr::GetInstance()->getLogger("root"))
  // << "sdv" << std::endl; spadger::Logger::ptr l =
  // spadger::LoggerMgr::GetInstance()->getLogger("root"); std::cout <<
  // l.use_count() << std::endl; l = spadger::Logger::ptr(new
  // spadger::Logger("root")); l->setLevel(spadger::LogLevel::DEBUG);
  // f_appender1->setFormatter(formatter);
  // l->addAppender(f_appender1);
  // std::cout << l->getName() << std::endl;
  // SPADGER_LOG_INFO(l) << "sdv" << std::endl;

  // ############################## 测试 log ###############################
  // spadger::Logger::ptr logger(new Logger());
  // std::cout << logger->getName() << std::endl;

  // return 0;
  spadger::Logger::ptr logger =
      spadger::SingleLoggerMgr::GetInstance()->getLogger("hello");
  logger->setLevel(spadger::LogLevel::DEBUG);

  spadger::LogFormat::ptr formatter(new spadger::LogFormat("[%d %c] %m"));
  spadger::StdoutLogAppender::ptr appender(new spadger::StdoutLogAppender());
  appender->setFormatter(formatter, true);
  appender->setLevel(spadger::LogLevel::ERROR);
  logger->addAppender(appender);

  spadger::LogEvent::ptr event(new spadger::LogEvent(
      logger, spadger::LogLevel::DEBUG, __FILE__, __LINE__, 0,
      spadger::GetThreadId(), spadger::GetFiberId(), time(0), "thread1"));
  event->getSS() << "log1 text" << std::endl;

  logger->info(event->getLevel(), event);

  SPADGER_LOG_INFO(logger) << "log2 text" << std::endl;

  return 0;
}
