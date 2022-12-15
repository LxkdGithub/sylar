/*
 * @Author: lxk
 * @Date: 2022-10-10 21:27:44
 * @LastEditors: lxk
 * @LastEditTime: 2022-10-11 17:51:19
 */
#include "config.h"
#include "log.h"
#include "util.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace spadger {
static spadger::Logger::ptr g_logger = SPADGER_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
  RWMutexType::ReadLock lock(GetMutex());
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

static void
ListAllMember(const std::string &prefix, const YAML::Node &node,
              std::list<std::pair<std::string, const YAML::Node>> &output) {
  if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
      std::string::npos) {
    SPADGER_LOG_ERROR(g_logger)
        << "Config invalid name: " << prefix << " : " << node;
    return;
  }
  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); it++) {
      // Recursive search.
      ListAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second, output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  // Put all pair<name, node> to all_nodes, include all layers.
  ListAllMember("", root, all_nodes);

  for (auto &i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }
    // std::cout << i.first << i.second << std::endl;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);
    // If we find the var, set the var's value to it->second.
    if (var) {
      if (i.second.IsScalar()) {
        var->fromString(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->fromString(ss.str());
      }
    }
  }
}

static spadger::Mutex s_mutex;
static std::map<std::string, uint64_t> s_file2modifytime;

// default val should not occur in the param list of function definition.
void Config::LoadFromConfDir(const std::string &path, bool force) {
  std::string absolute_path =
      spadger::EnvMgr::GetInstance()->getAbsolutePath(path);
  std::cout << absolute_path << std::endl;
  std::vector<std::string> files;
  FSUtil::ListAllFile(files, absolute_path, ".yml");

  for (auto &i : files) {
    {
      struct stat st;
      lstat(i.c_str(), &st);
      spadger::Mutex::Lock lock();
      if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
        continue;
      }
      s_file2modifytime[i] = st.st_mtime;
    }
    try {
      YAML::Node node = YAML::LoadFile(i);
      LoadFromYaml(node);
      SPADGER_LOG_INFO(g_logger) << "LoadConfFile file=" << i << " ok";
    } catch (...) {
      SPADGER_LOG_ERROR(g_logger) << "LoadConfFile file=" << i << " failed";
    }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  RWMutexType::WriteLock lock(GetMutex());
  ConfigVarMap &m = GetDatas();
  for (auto it = m.begin(); it != m.end(); it++) {
    cb(it->second);
  }
}

} // namespace spadger