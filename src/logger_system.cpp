/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/logger_system.hpp>

#include <set>

#include <soralog/group.hpp>
#include <soralog/logger.hpp>

namespace soralog {

  void LoggerSystem::makeGroup(std::string name,
                               const std::optional<std::string> &parent,
                               const std::optional<std::string> &sink,
                               const std::optional<Level> &level) {
    auto group =
        std::make_shared<Group>(*this, std::move(name), parent, sink, level);
    if (not fallback_group_) {
      fallback_group_ = group;
      groups_["*"] = group;
    }
    groups_[group->name()] = std::move(group);
  }

  Configurator::Result LoggerSystem::configure() {
    if (is_configured_) {
      throw std::logic_error("LoggerSystem is already configured");
    }
    is_configured_ = true;
    return configurator_->applyOn(*this);
  }

  std::shared_ptr<Logger> LoggerSystem::getLogger(
      std::string logger_name, const std::string &group_name,
      const std::optional<std::string> &sink_name,
      const std::optional<Level> &level) {
    if (not is_configured_) {
      throw std::logic_error("LoggerSystem is not yet configured");
    }

    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        return std::move(logger);
      }
    }

    auto group = getGroup(group_name);
    if (group == nullptr) {
      group = fallback_group_;
      assert(group != nullptr);  // At least fallback group must be existing
      auto logger = std::make_shared<Logger>(*this, "Soralog", group);
      logger->warn(
          "Group '{}' for logger '{}' is not found. "
          "Fallback group will be used. Use '*' group for using default one",
          group_name, logger_name);
    }

    auto logger = std::make_shared<Logger>(*this, std::move(logger_name),
                                           std::move(group));

    if (sink_name.has_value()) {
      logger->setSink(sink_name.value());
    }

    if (level.has_value()) {
      logger->setLevel(level.value());
    }

    loggers_[logger->name()] = logger;
    return logger;
  }

  [[nodiscard]] std::shared_ptr<Sink> LoggerSystem::getSink(
      const std::string &sink_name) {
    auto it = sinks_.find(sink_name);
    if (it == sinks_.end()) {
      return {};
      it = sinks_.find("default");
      assert(it != sinks_.end());
    }
    return it->second;
  }

  [[nodiscard]] std::shared_ptr<Group> LoggerSystem::getGroup(
      const std::string &group_name) {
    auto it = groups_.find(group_name);
    if (it == groups_.end()) {
      return {};
      it = groups_.find("default");
      assert(it != groups_.end());
    }
    return it->second;
  }

  void LoggerSystem::setParentForGroup(const std::shared_ptr<Group> &group,
                                       const std::shared_ptr<Group> &parent) {
    group->setParentGroup(parent);

    std::map<std::shared_ptr<const Group>, int> passed_groups;

    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    std::function<size_t(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> size_t {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current->isLevelOverriden() && current->isSinkOverriden()) {
        return -1;
      }
      if (current == group) {
        return 0;
      }
      if (not current->parent()) {
        return -1;
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setParentGroup(changing_group->parent());
      }
    }

    for (auto it = loggers_.begin(); it != loggers_.end();) {
      auto cit = *++it;
      if (auto logger = cit.second.lock()) {
        if (auto it2 = passed_groups.find(logger->group());
            it2 != passed_groups.end()) {
          if (it2->second != -1) {
            logger->setGroup(logger->group());
          }
        }
      } else {
        loggers_.erase(cit.first);
      }
    }
  }

  void LoggerSystem::setSinkForGroup(
      const std::shared_ptr<Group> &group,
      std::optional<std::shared_ptr<Sink>> sink) {
    if (sink) {
      group->setSink(*sink);
    } else {
      group->resetSink();
    }

    std::map<std::shared_ptr<const Group>, int> passed_groups;

    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    std::function<size_t(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> size_t {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current->isSinkOverriden()) {
        return -1;
      }
      if (current == group) {
        return 0;
      }
      if (not current->parent()) {
        return -1;
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setSinkFromGroup(changing_group->parent());
      }
    }

    for (auto it = loggers_.begin(); it != loggers_.end();) {
      auto cit = *++it;
      if (auto logger = cit.second.lock()) {
        if (auto it2 = passed_groups.find(logger->group());
            it2 != passed_groups.end()) {
          if (it2->second != -1) {
            logger->setSinkFromGroup(logger->group());
          }
        }
      } else {
        loggers_.erase(cit.first);
      }
    }
  }

  void LoggerSystem::setLevelForGroup(const std::shared_ptr<Group> &group,
                                      std::optional<Level> level) {
    if (level) {
      group->setLevel(*level);
    } else {
      group->resetLevel();
    }

    std::map<std::shared_ptr<const Group>, int> passed_groups;

    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    std::function<size_t(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> size_t {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current->isLevelOverriden()) {
        return -1;
      }
      if (current == group) {
        return 0;
      }
      if (not current->parent()) {
        return -1;
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setLevelFromGroup(changing_group->parent());
      }
    }

    for (auto it = loggers_.begin(); it != loggers_.end();) {
      auto cit = *++it;
      if (auto logger = cit.second.lock()) {
        if (auto it2 = passed_groups.find(logger->group());
            it2 != passed_groups.end()) {
          if (it2->second != -1) {
            logger->setLevelFromGroup(logger->group());
          }
        }
      } else {
        loggers_.erase(cit.first);
      }
    }
  }

  void LoggerSystem::setSinkForLogger(
      const std::shared_ptr<Logger> &logger,
      std::optional<std::shared_ptr<Sink>> sink) {
    if (sink) {
      logger->setSink(std::move(*sink));
    } else {
      logger->resetSink();
    }
  }

  void LoggerSystem::setLevelForLogger(const std::shared_ptr<Logger> &logger,
                                       std::optional<Level> level) {
    if (level) {
      logger->setLevel(*level);
    } else {
      logger->resetLevel();
    }
  }

  void LoggerSystem::setParentForGroup(const std::string &group_name,
                                       const std::string &parent_name) {
    auto it1 = groups_.find(group_name);
    if ( it1 == groups_.end()) {
      return;
    }
    auto& group = it1->second;

    auto it2 = groups_.find(parent_name);
    if ( it2 == groups_.end()) {
      return;
    }
    auto& parent = it2->second;

    // Check for recursion
    for (auto current = parent->parent(); current != nullptr;
         current = current->parent()) {
      if (current == group) {
        // Cyclic parentness is detected
        return;
      }
    }

    setParentForGroup(group, parent);
  }

  void LoggerSystem::setSinkForGroup(const std::string &group_name,
                                     const std::string &sink_name) {
    auto sink = getSink(sink_name);
    if (not sink) {
      return;
    }
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto& group = it->second;
      setSinkForGroup(group, std::move(sink));
    }
  }

  void LoggerSystem::resetSinkForGroup(const std::string &group_name) {
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto& group = it->second;
      setSinkForGroup(group, {});
    }
  }

  void LoggerSystem::setLevelForGroup(const std::string &group_name,
                                      Level level) {
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto& group = it->second;
      setLevelForGroup(group, level);
    }
  }

  void LoggerSystem::resetLevelForGroup(const std::string &group_name) {
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto& group = it->second;
      setLevelForGroup(group, {});
    }
  }

  void LoggerSystem::setGroupForLogger(const std::string &logger_name,
                                       const std::string &group_name) {
    if (auto group = getGroup(group_name)) {
      if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
        if (auto logger = it->second.lock()) {
          logger->setGroup(std::move(group));
        } else {
          loggers_.erase(it);
        }
      }
    }
  }

  void LoggerSystem::setSinkForLogger(const std::string &logger_name,
                                      const std::string &sink_name) {
    if (auto sink = getSink(sink_name)) {
      if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
        if (auto logger = it->second.lock()) {
          logger->setSink(std::move(sink));
        } else {
          loggers_.erase(it);
        }
      }
    }
  }

  void LoggerSystem::resetSinkForLogger(const std::string &logger_name) {
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        logger->setSinkFromGroup(logger->group());
      } else {
        loggers_.erase(it);
      }
    }
  }

  void LoggerSystem::setLevelForLogger(const std::string &logger_name,
                                       Level level) {
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        logger->setLevel(level);
      } else {
        loggers_.erase(it);
      }
    }
  }

  void LoggerSystem::resetLevelForLogger(const std::string &logger_name) {
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        logger->setLevelFromGroup(logger->group());
      } else {
        loggers_.erase(it);
      }
    }
  }
}  // namespace soralog
