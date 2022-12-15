/*
 * @Author: lxk
 * @Date: 2022-10-11 10:16:55
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-12 11:49:01
 */
#include "config.h"
// #include "env.h"
#include "log.h"
#include <iostream>
#include <yaml-cpp/yaml.h>
// 整个config流程
/*
            YAML 读取
.yml file  <-----------> YAML::Node(map[string]string)<--|
                                                         |
                                                         |LexicalCast<A,B>转化
                                                         |
ConfigVar:各种结构(int, vector, list, 自定义结构)<--------|
(output的时候都是string类型)所以输出的时候也需要转化为string



*/

#if 1
spadger::ConfigVar<int>::ptr g_int_value_config =
    spadger::Config::Lookup("system.port", (int)8080, "system port");

spadger::ConfigVar<float>::ptr g_int_valuex_config =
    spadger::Config::Lookup("system.port", (float)8080, "system port");

spadger::ConfigVar<float>::ptr g_float_value_config =
    spadger::Config::Lookup("system.value", (float)10.2f, "system value");

spadger::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    spadger::Config::Lookup("system.int_vec", std::vector<int>{1, 2},
                            "system int vec");

spadger::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    spadger::Config::Lookup("system.int_list", std::list<int>{1, 2},
                            "system int list");

spadger::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    spadger::Config::Lookup("system.int_set", std::set<int>{1, 2},
                            "system int set");

spadger::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    spadger::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2},
                            "system int uset");

spadger::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    spadger::Config::Lookup("system.str_int_map",
                            std::map<std::string, int>{{"k", 2}},
                            "system str int map");

spadger::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config =
        spadger::Config::Lookup("system.str_int_umap",
                                std::unordered_map<std::string, int>{{"k", 2}},
                                "system str int map");

void print_yaml(const YAML::Node &node, int level) {
  if (node.IsScalar()) {
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
        << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type()
        << " - " << level;
  } else if (node.IsNull()) {
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
        << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - "
        << level;
  } else if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
          << std::string(level * 4, ' ') << it->first << " - "
          << it->second.Type() << " - " << level;
      print_yaml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
          << std::string(level * 4, ' ') << i << " - " << node[i].Type()
          << " - " << level;
      print_yaml(node[i], level + 1);
    }
  }
}

void test_yaml() {
  // 仅测试YAML读取(YAML的作用是从文件读取结果为结构体，但是结构体的KV都是Str)
  YAML::Node root = YAML::LoadFile("/home/lxk/Lab/Sylar/bin/conf/log.yml");
  // print_yaml(root, 0);
  //   SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << root.Scalar();

  SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << root["test"].IsDefined();
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << root["logs"].IsDefined();
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << root;
}

void test_config() {
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "before: " << g_int_value_config->getValue();
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "before: " << g_float_value_config->toString();

#define XX(g_var, name, prefix)                                                \
  {                                                                            \
    auto &v = g_var->getValue();                                               \
    for (auto &i : v) {                                                        \
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << #prefix " " #name ": " << i;     \
    }                                                                          \
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT())                                       \
        << #prefix " " #name " yaml: " << g_var->toString();                   \
  }

#define XX_M(g_var, name, prefix)                                              \
  {                                                                            \
    auto &v = g_var->getValue();                                               \
    for (auto &i : v) {                                                        \
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT())                                     \
          << #prefix " " #name ": {" << i.first << " - " << i.second << "}";   \
    }                                                                          \
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT())                                       \
        << #prefix " " #name " yaml: " << g_var->toString();                   \
  }

  XX(g_int_vec_value_config, int_vec, before);
  XX(g_int_list_value_config, int_list, before);
  XX(g_int_set_value_config, int_set, before);
  XX(g_int_uset_value_config, int_uset, before);
  XX_M(g_str_int_map_value_config, str_int_map, before);
  XX_M(g_str_int_umap_value_config, str_int_umap, before);

  YAML::Node root = YAML::LoadFile("/home/lxk/Lab/Sylar/bin/conf/log.yml");
  spadger::Config::LoadFromYaml(root);

  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "after: " << g_int_value_config->getValue();
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "after: " << g_float_value_config->toString();

  XX(g_int_vec_value_config, int_vec, after);
  XX(g_int_list_value_config, int_list, after);
  XX(g_int_set_value_config, int_set, after);
  XX(g_int_uset_value_config, int_uset, after);
  XX_M(g_str_int_map_value_config, str_int_map, after);
  XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

#endif

class Person {
public:
  Person(){};
  std::string m_name;
  int m_age = 0;
  bool m_sex = 0;

  std::string toString() const {
    std::stringstream ss;
    ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex
       << "]";
    return ss.str();
  }

  bool operator==(const Person &oth) const {
    return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
  }
};

namespace spadger {

template <> class LexicalCast<std::string, Person> {
public:
  Person operator()(const std::string &v) {
    YAML::Node node = YAML::Load(v);
    Person p;
    p.m_name = node["name"].as<std::string>();
    p.m_age = node["age"].as<int>();
    p.m_sex = node["sex"].as<bool>();
    return p;
  }
};

template <> class LexicalCast<Person, std::string> {
public:
  std::string operator()(const Person &p) {
    YAML::Node node;
    node["name"] = p.m_name;
    node["age"] = p.m_age;
    node["sex"] = p.m_sex;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

} // namespace spadger

spadger::ConfigVar<Person>::ptr g_person =
    spadger::Config::Lookup("class.person", Person(), "system person");

spadger::ConfigVar<std::map<std::string, Person>>::ptr g_person_map =
    spadger::Config::Lookup("class.map", std::map<std::string, Person>(),
                            "system person");

spadger::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr
    g_person_vec_map =
        spadger::Config::Lookup("class.vec_map",
                                std::map<std::string, std::vector<Person>>(),
                                "system person");

void test_class() {
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "before: " << g_person->getValue().toString() << " - "
      << g_person->toString();

#define XX_PM(g_var, prefix)                                                   \
  {                                                                            \
    auto m = g_person_map->getValue();                                         \
    for (auto &i : m) {                                                        \
      SPADGER_LOG_INFO(SPADGER_LOG_ROOT())                                     \
          << prefix << ": " << i.first << " - " << i.second.toString();        \
    }                                                                          \
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << prefix << ": size=" << m.size();   \
  }

  g_person->addListener([](const Person &old_value, const Person &new_value) {
    SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
        << "old_value=" << old_value.toString()
        << " new_value=" << new_value.toString();
  });

  XX_PM(g_person_map, "class.map before");
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "before: " << g_person_vec_map->toString();

  YAML::Node root = YAML::LoadFile("/home/lxk/Lab/Sylar/bin/conf/log.yml");
  spadger::Config::LoadFromYaml(root);

  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "after: " << g_person->getValue().toString() << " - "
      << g_person->toString();
  XX_PM(g_person_map, "class.map after");
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
      << "after: " << g_person_vec_map->toString();
}

void test_log() {
  static spadger::Logger::ptr system_log = SPADGER_LOG_NAME("system");
  SPADGER_LOG_INFO(system_log) << "hello system" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << spadger::SingleLoggerMgr::GetInstance()->toYamlString()
            << std::endl;
  YAML::Node root = YAML::LoadFile("/home/lxk/Lab/Sylar/bin/conf/log.yml");
  spadger::Config::LoadFromYaml(root);
  std::cout << "=============" << std::endl;
  std::cout << spadger::SingleLoggerMgr::GetInstance()->toYamlString()
            << std::endl;
  std::cout << "=============" << std::endl;
  std::cout << root << std::endl;
  SPADGER_LOG_INFO(system_log) << "hello system";
  system_log->setFormatter("%d - %m%n");
  std::cout << spadger::SingleLoggerMgr::GetInstance()->toYamlString()
            << std::endl;
  SPADGER_LOG_INFO(system_log) << "hello system";
  SPADGER_LOG_INFO(SPADGER_LOG_ROOT()) << "I am root.";
}

void test_loadconf() { spadger::Config::LoadFromConfDir("conf"); }

int main(int argc, char **argv) {
  //   test_yaml();
  //   test_config();
  //   test_class();
  test_log();
  //   spadger::EnvMgr::GetInstance()->init(argc, argv);
  //   std::cout << "--------------------" << std::endl;
  //   test_loadconf();
  //   std::cout << " ==== " << std::endl;
  //   sleep(10);
  //   test_loadconf();
  //   return 0;
  //   spadger::Config::Visit([](spadger::ConfigVarBase::ptr var) {
  //     SPADGER_LOG_INFO(SPADGER_LOG_ROOT())
  //         << "name=" << var->getName() << " description=" <<
  //         var->getDescription()
  //         << " typename=" << var->getTypeName() << " value=" <<
  //         var->toString();
  //   });

  return 0;
}