#ifndef __SPADGER_CONFIG_H__
#define __SPADGER_CONFIG_H__

#include "env.h"
#include "log.h"
#include <boost/lexical_cast.hpp>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <yaml-cpp/node/type.h>
#include <yaml-cpp/yaml.h>

namespace spadger {

// #####################################################################
// #######################  ConfigVarBase  #############################

class ConfigVarBase {
public:
  typedef std::shared_ptr<ConfigVarBase> ptr;
  ConfigVarBase(const std::string &name, const std::string &description = "")
      : m_name(name), m_description(description) {
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }
  virtual ~ConfigVarBase() {}

  const std::string &getName() { return m_name; }
  const std::string &getDescription() { return m_description; }

  // 将key-value转化为string 或者从string得到key-value
  virtual std::string toString() = 0;
  virtual bool fromString(const std::string &val) = 0;
  virtual std::string getTypeName() const = 0;

protected:
  std::string m_name;
  std::string m_description;
};

// #####################################################################
// ########################  Cast class ################################
template <class F, class T> class LexicalCast {
public:
  /**
   * @description: 类型转化
   * @param {F&} f 转换前的类型
   * @return {*} 转化后的类型
   * 不可
   */
  T operator()(const F &f) { return boost::lexical_cast<T>(f); }
};

// Yaml::Load只接受string 所以无论什么需要转化为yaml格式 都需要先化为string ->
// node.push_back() -> stringstream -> string 偏特化
template <typename T> class LexicalCast<std::string, std::vector<T>> {
public:
  std::vector<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    std::vector<T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << *it;
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <typename T> class LexicalCast<std::vector<T>, std::string> {
public:
  std::string operator()(const std::vector<T> &v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// map
template <typename T> class LexicalCast<std::string, std::map<std::string, T>> {
public:
  std::map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    std::map<std::string, T> m;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      // it->second是什么格式 还要转化为string(我们只有T和string的互转)
      // 把每个配置项转化为pair<string, T>
      ss.str("");
      ss << it->second;
      // Scalar 返回string
      m.insert(std::make_pair(it->first.Scalar(),
                              LexicalCast<std::string, T>()(ss.str())));
    }
    return m;
  }
};

// map
template <typename T> class LexicalCast<std::map<std::string, T>, std::string> {
public:
  std::string operator()(const std::map<std::string, T> &v) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto it = v.begin(); it != v.end(); ++it) {
      node[it->first] = YAML::Load(LexicalCast<T, std::string>()(it->second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// list
template <class T> class LexicalCast<std::string, std::list<T>> {
public:
  std::list<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::list<T> 转换成 YAML String)
 */
template <class T> class LexicalCast<std::list<T>, std::string> {
public:
  std::string operator()(const std::list<T> &v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::set<T>)
 */
template <class T> class LexicalCast<std::string, std::set<T>> {
public:
  std::set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template <class T> class LexicalCast<std::set<T>, std::string> {
public:
  std::string operator()(const std::set<T> &v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

/**
 * @brief 类型转换模板类片特化(YAML String 转换成 std::unordered_set<T>)
 */
template <class T> class LexicalCast<std::string, std::unordered_set<T>> {
public:
  std::unordered_set<T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_set<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_set<T> 转换成 YAML String)
 */
template <class T> class LexicalCast<std::unordered_set<T>, std::string> {
public:
  std::string operator()(const std::unordered_set<T> &v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (auto &i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// unordered map
/**
 * @brief 类型转换模板类片特化(YAML String 转换成
 * std::unordered_map<std::string, T>)
 */
template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
  std::unordered_map<std::string, T> operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    typename std::unordered_map<std::string, T> vec;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      vec.insert(std::make_pair(it->first.Scalar(),
                                LexicalCast<std::string, T>()(ss.str())));
    }
    return vec;
  }
};

/**
 * @brief 类型转换模板类片特化(std::unordered_map<std::string, T> 转换成 YAML
 * String)
 */
template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
  std::string operator()(const std::unordered_map<std::string, T> &v) {
    YAML::Node node(YAML::NodeType::Map);
    for (auto &i : v) {
      node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

// ##########################################################################
// ########################  ConfigVar class ################################

template <class T, class ToStr = LexicalCast<T, std::string>,
          class FromStr = LexicalCast<std::string, T>>
class ConfigVar : public ConfigVarBase {
public:
  typedef RWMutex RWMutexType;
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_value, const T &new_value)>
      on_change_cb; // 这种命名方式
  ConfigVar(const std::string &name, const T &default_value,
            const std::string &description = "")
      : ConfigVarBase(name, description), m_val(default_value) {}

  std::string toString() override {
    try {
      // return boost::lexical_cast<std::string>(m_val);
      // Read Lock
      RWMutexType::ReadLock lock(m_mutex);
      return ToStr()(m_val);
    } catch (std::exception &e) {
      SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())
          << "ConfigVarException::toString" << e.what() << "convert"
          << typeid(m_val).name() << "to string";
    }
    return "";
  }
  bool fromString(const std::string &val) override {
    try {
      // m_val = boost::lexical_cast<T>(val);
      setValue(FromStr()(val)); // lock in setValue function.
      return true;
    } catch (std::exception &e) {
      SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())
          << "ConfigVarException::toString" << e.what() << "convert"
          << "from string to " << typeid(m_val).name();
    }
    return false;
  }

  const T getValue() {
    RWMutexType::ReadLock lock(m_mutex);
    return m_val;
  }

  void setValue(const T &v) {
    {
      // Read Lock
      RWMutexType::ReadLock lock(m_mutex);
      if (m_val == v) {
        return;
      }
      for (auto &i : m_cbs) {
        i.second(m_val, v);
      }
    }
    // Write Lock
    RWMutexType::WriteLock lock(m_mutex);
    m_val = v;
  }

  std::string getTypeName() const override { return typeid(T).name(); }

  // 下面是几个cb function 的operator : add del get clear
  uint64_t addListener(on_change_cb cb) {
    // 使用static的话既不需要每次自己生成了，可以保证一致性
    static uint64_t s_fun_id = 0;
    RWMutexType::WriteLock lock(m_mutex);
    ++s_fun_id;
    m_cbs[s_fun_id] = cb;
    return s_fun_id;
  }

  void delListener(uint64_t key) {
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.erase(key);
  }

  on_change_cb getListener(uint64_t key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_cbs.find(key);
    return it == m_cbs.end() ? nullptr : it->second;
  }

  void clearListener() {
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.clear();
  }

private:
  RWMutexType m_mutex; // 使用Type的原因是可以指定锁的类型？
  T m_val;
  // 为什么使用map呢? 因为function没有比较函数，所以使用id标识
  std::map<uint64_t, on_change_cb> m_cbs;
};

// ##########################################################################
// ########################  Config class ################################

class Config {
public:
  typedef RWMutex RWMutexType;
  typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

  // 1. Lookup(string name); --------------------------------
  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    if (it == GetDatas().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
  }

  // 2. Lookup(string name, default_val, desp); --------------------------------
  template <class T>
  static typename ConfigVar<T>::ptr
  Lookup(const std::string &name, const T &default_val,
         const std::string &description = "") {
    RWMutexType::WriteLock lock(GetMutex());
    // 1. Find, if found, return directly.
    auto it = GetDatas().find(name);
    if (it != GetDatas().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp) {
        SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
            << "Lookup name=" << name << " exists.";
        return tmp;
      } else {
        SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())
            << "Lookup name=" << name << " exists but type not "
            << typeid(T).name() << " realType=" << it->second->getTypeName()
            << " " << it->second->toString();
        return nullptr;
      }
    }

    // 2. Valid Check. If valid, insert it.
    if (name.find_first_not_of("abcdefghijklmnopqrstuvwzyxABCDEFGHIJKLMNOPQRSTU"
                               "VWXYZ._0123456789") != std::string::npos) {

      SPADGER_LOG_ERROR(SPADGER_LOG_ROOT())
          << "Lookup name=" << name << " invalid";
      throw std::invalid_argument(name);
    }
    typename ConfigVar<T>::ptr v(
        new ConfigVar<T>(name, default_val, description));
    GetDatas()[name] = v;
    return v;
  }
  // 3. Load From YAML::Node --------------------------------
  static void LoadFromYaml(const YAML::Node &root);
  // 4. Load from Dir
  static void LoadFromConfDir(const std::string &path, bool force = false);
  // 5. Find ConfigVar and return the base class of var.
  static ConfigVarBase::ptr LookupBase(const std::string &name);
  // 6. Traverse all ConfigVars.
  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
  // Return all ConfigVars. (static vars in static method means that the var is
  // static in class.)
  static ConfigVarMap &GetDatas() {
    static ConfigVarMap s_datas;
    return s_datas;
  }
  // Return staticMutex(1) in function(2), why?
  // 1.  Because the s_data is a member of class stead of object,
  //     so the mutex should also a member of class.
  // 2.  And we should make sure the construction of s_mutex is in before of
  //     construction of s_datas. So we can not use class-static var to declare
  //     s_mutex var.
  static RWMutexType &GetMutex() {
    static RWMutexType s_mutex;
    return s_mutex;
  }
};

} // namespace spadger

#endif
