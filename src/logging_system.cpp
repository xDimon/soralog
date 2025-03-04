/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/logging_system.hpp>

#include <cassert>
#include <functional>
#include <ranges>
#include <set>

#include <soralog/group.hpp>
#include <soralog/impl/sink_to_nowhere.hpp>
#include <soralog/logger.hpp>

using std::literals::string_literals::operator""s;

namespace soralog {

  LoggingSystem::LoggingSystem() : is_configured_(true) {
    // Create a fallback sink that discards all logs
    makeSink<SinkToNowhere>("*");
    makeGroup("*", {}, "*", Level::OFF);
    ;
  }

  LoggingSystem::LoggingSystem(std::shared_ptr<Configurator> configurator)
      : configurator_(std::move(configurator)) {
    // Create a fallback sink that discards all logs
    makeSink<SinkToNowhere>("*");
  }

  std::shared_ptr<Group> LoggingSystem::makeGroup(
      std::string name,
      const std::optional<std::string> &parent,
      const std::optional<std::string> &sink,
      const std::optional<Level> &level) {
    // Create a new group with the provided parameters
    auto group =
        std::make_shared<Group>(*this, std::move(name), parent, sink, level);

    std::lock_guard guard(mutex_);

    // Ensure that the fallback group ("*") is always present
    if (groups_.find("*") == groups_.end()) {
      groups_["*"] = group;
    }

    // Store the new group
    groups_[group->name()] = group;
    return group;
  }

  bool LoggingSystem::setFallbackGroup(const std::string &group_name) {
    std::lock_guard guard(mutex_);
    auto it = groups_.find(group_name);
    if (it == groups_.end()) {
      return false;
    }
    // Update the fallback group
    groups_["*"] = it->second;
    return true;
  }

  std::shared_ptr<Group> LoggingSystem::getFallbackGroup() const {
    auto it = groups_.find("*");
    if (it == groups_.end()) {
      return {};
    }
    return it->second;
  }

  Configurator::Result LoggingSystem::configure() {
    std::lock_guard guard(mutex_);

    if (is_configured_) {
      throw std::logic_error(
          "LoggerSystem is already configured "
          "or requires manual configuration");
    }
    is_configured_ = true;

    Configurator::Result result;
    try {
      result = configurator_->applyOn(*this);
    } catch (const std::exception &exception) {
      result.message += "E: Configure failed: "s + exception.what() + "; "
                      + "Logging system is unworkable\n";
      result.has_error = true;
      return result;
    }

    // Ensure that at least one group is defined
    if (groups_.empty()) {
      result.message +=
          "E: No group is defined; Logging system is unworkable\n";
      result.has_error = true;
      return result;
    }

    // Check if any group has an undefined sink
    for (auto &[name, group] : groups_) {
      if (name == "*") {
        continue;
      }
      if (group->sink()->name() == "*") {
        result.message += "W: Group '" + name + "' has undefined sink; "
                        + "Sink to nowhere will be used\n";
        result.has_warning = true;
      }
    }

    return result;
  }

  std::shared_ptr<Logger> LoggingSystem::getLogger(
      std::string logger_name,
      const std::string &group_name,
      const std::optional<std::string> &sink_name,
      const std::optional<Level> &level) {
    std::lock_guard guard(mutex_);

    if (not is_configured_) {
      throw std::logic_error("LoggerSystem is not yet configured");
    }

    // If the logger already exists, return it
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        return std::move(logger);
      }
    }

    // Handle deprecated use of default group "*"
    if (group_name == "*") {
      std::string warn_msg =
          "Default group (calling with name '*') is deprecated and should not "
          "be used anymore; Define an existing group explicitly";
#ifndef NDEBUG
      throw std::runtime_error(warn_msg);
#endif
      auto group = getFallbackGroup();
      auto logger = std::make_shared<Logger>(*this, "Soralog", group);
      logger->warn(warn_msg);
    }

    // Get the specified group, or fall back to the default
    auto group = getGroup(group_name);
    if (group == nullptr) {
      group = getFallbackGroup();
      assert(group != nullptr);  // Fallback group must always exist
      auto logger = std::make_shared<Logger>(*this, "Soralog", group);
      logger->warn(
          "Group '{}' for logger '{}' is not found. "
          "Fallback group will be used (currently '{}').",
          group_name,
          logger_name,
          group->name());
    }

    // Create a new logger instance
    auto logger = std::make_shared<Logger>(
        *this, std::move(logger_name), std::move(group));

    // Apply optional overrides
    if (sink_name.has_value()) {
      logger->setSink(sink_name.value());
    }
    if (level.has_value()) {
      logger->setLevel(level.value());
    }

    loggers_[logger->name()] = logger;
    return logger;
  }

  [[nodiscard]] std::shared_ptr<Sink> LoggingSystem::getSink(
      const std::string &sink_name) {
    std::lock_guard guard(mutex_);
    auto it = sinks_.find(sink_name);
    if (it == sinks_.end()) {
      return {};
    }
    return it->second;
  }

  [[nodiscard]] std::shared_ptr<Group> LoggingSystem::getGroup(
      const std::string &group_name) {
    std::lock_guard guard(mutex_);
    auto it = groups_.find(group_name);
    if (it == groups_.end()) {
      return {};
    }
    return it->second;
  }

  void LoggingSystem::setParentOfGroup(const std::shared_ptr<Group> &group,
                                       const std::shared_ptr<Group> &parent) {
    assert(group != nullptr);

    // If the new parent has the current group as its parent, unset its parent
    // to avoid cycles
    if (parent && parent->parent() == group) {
      parent->unsetParentGroup();
    }
    group->setParentGroup(parent);

    // Track groups that need updates
    std::map<std::shared_ptr<const Group>, int> passed_groups;
    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    // Recursive function to find affected groups and their update order
    std::function<int(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> int {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current == group) {
        return 0;
      }
      if (current->isLevelOverridden() && current->isSinkOverridden()) {
        return -1;  // No need to update this group
      }
      if (not current->parent()) {
        return -1;  // No parent means it's at the root level
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      if (n >= affecting_groups.size()) {
        affecting_groups.resize(n + 1);
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    std::lock_guard guard(mutex_);

    // Identify groups that need to update their parent
    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    // Update affected groups in the correct order
    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setParentGroup(changing_group->parent());
      }
    }

    // Update all loggers belonging to affected groups
    for (auto it = loggers_.begin(); it != loggers_.end();) {
      if (auto logger = it->second.lock()) {
        if (auto it2 = passed_groups.find(logger->group());
            it2 != passed_groups.end()) {
          if (it2->second != -1) {
            logger->setGroup(logger->group());
          }
        }
        ++it;
      } else {
        it = loggers_.erase(it);  // Remove expired loggers
      }
    }
  }

  void LoggingSystem::setSinkOfGroup(
      const std::shared_ptr<Group> &group,
      std::optional<std::shared_ptr<Sink>> sink) {
    assert(group != nullptr);

    // Apply the new sink to the group
    if (sink) {
      group->setSink(*sink);
    } else {
      group->resetSink();
    }

    // Track groups that need sink updates
    std::map<std::shared_ptr<const Group>, int> passed_groups;
    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    // Identify groups that inherit their sink from the affected group
    std::function<int(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> int {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current == group) {
        return 0;
      }
      if (current->isSinkOverridden()) {
        return -1;  // Skip if sink is explicitly set
      }
      if (not current->parent()) {
        return -1;
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      if (n >= affecting_groups.size()) {
        affecting_groups.resize(n + 1);
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    std::lock_guard guard(mutex_);

    // Identify affected groups
    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    // Update affected groups in order
    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setSinkFromGroup(changing_group->parent());
      }
    }

    // Update all loggers that inherit their sink
    for (auto it = loggers_.begin(); it != loggers_.end();) {
      if (auto logger = it->second.lock()) {
        if (not logger->isSinkOverridden()) {
          if (auto it2 = passed_groups.find(logger->group());
              it2 != passed_groups.end()) {
            if (it2->second != -1) {
              logger->setSinkFromGroup(logger->group());
            }
          }
        }
        ++it;
      } else {
        it = loggers_.erase(it);
      }
    }
  }

  void LoggingSystem::setLevelOfGroup(const std::shared_ptr<Group> &group,
                                      std::optional<Level> level) {
    assert(group != nullptr);

    // Apply the new log level
    if (level) {
      group->setLevel(*level);
    } else {
      group->resetLevel();
    }

    // Track groups that need log level updates
    std::map<std::shared_ptr<const Group>, int> passed_groups;
    std::vector<std::set<std::shared_ptr<Group>>> affecting_groups;

    // Identify groups that inherit their log level
    std::function<int(const std::shared_ptr<const Group> &)> fn =
        [&](const std::shared_ptr<const Group> &current) mutable -> int {
      if (auto it = passed_groups.find(current); it != passed_groups.end()) {
        return it->second;
      }
      if (current == group) {
        return 0;
      }
      if (current->isLevelOverridden()) {
        return -1;  // Skip if log level is explicitly set
      }
      if (not current->parent()) {
        return -1;
      }
      auto n = fn(current->parent());
      if (n == -1) {
        return -1;
      }
      if (n >= affecting_groups.size()) {
        affecting_groups.resize(n + 1);
      }
      affecting_groups[n].emplace(std::const_pointer_cast<Group>(current));
      return ++n;
    };

    std::lock_guard guard(mutex_);

    // Identify affected groups
    for (const auto &it : groups_) {
      auto n = fn(it.second);
      passed_groups[it.second] = n;
    }

    // Update affected groups in order
    for (const auto &stage : std::move(affecting_groups)) {
      for (const auto &changing_group : stage) {
        changing_group->setLevelFromGroup(changing_group->parent());
      }
    }

    // Update all loggers that inherit their log level
    for (auto it = loggers_.begin(); it != loggers_.end();) {
      if (auto logger = it->second.lock()) {
        if (not logger->isLevelOverridden()) {
          if (auto it2 = passed_groups.find(logger->group());
              it2 != passed_groups.end()) {
            if (it2->second != -1) {
              logger->setLevelFromGroup(logger->group());
            }
          }
        }
        ++it;
      } else {
        it = loggers_.erase(it);
      }
    }
  }

  void LoggingSystem::setSinkOfLogger(
      const std::shared_ptr<Logger> &logger,
      std::optional<std::shared_ptr<Sink>> sink) {
    assert(logger != nullptr);
    // If a new sink is provided, apply it; otherwise, reset to the group's sink
    if (sink) {
      logger->setSink(std::move(*sink));
    } else {
      logger->resetSink();
    }
  }

  void LoggingSystem::setLevelOfLogger(const std::shared_ptr<Logger> &logger,
                                       std::optional<Level> level) {
    assert(logger != nullptr);
    // If a new level is provided, apply it;
    // otherwise, reset to the group's level
    if (level) {
      logger->setLevel(*level);
    } else {
      logger->resetLevel();
    }
  }

  bool LoggingSystem::setParentOfGroup(const std::string &group_name,
                                       const std::string &parent_name) {
    std::lock_guard guard(mutex_);

    auto it1 = groups_.find(group_name);
    if (it1 == groups_.end()) {
      return false;
    }
    auto &group = it1->second;

    auto it2 = groups_.find(parent_name);
    if (it2 == groups_.end()) {
      return false;
    }
    auto &parent = it2->second;

    // Prevent cyclic parent-child relationships
    if (parent->parent() != group) {
      for (auto current = parent->parent(); current != nullptr;
           current = current->parent()) {
        if (current == group) {
          // Detected cyclic dependency, aborting
          return false;
        }
      }
    }

    setParentOfGroup(group, parent);
    return true;
  }

  bool LoggingSystem::unsetParentOfGroup(const std::string &group_name) {
    std::lock_guard guard(mutex_);

    auto it = groups_.find(group_name);
    if (it == groups_.end()) {
      return false;
    }
    auto &group = it->second;

    // Remove parent group reference
    setParentOfGroup(group, {});
    return true;
  }

  bool LoggingSystem::setSinkOfGroup(const std::string &group_name,
                                     const std::string &sink_name) {
    std::lock_guard guard(mutex_);
    auto sink = getSink(sink_name);
    if (not sink) {
      return false;
    }
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto &group = it->second;
      // Apply new sink to the group
      setSinkOfGroup(group, std::move(sink));
      return true;
    }
    return false;
  }

  bool LoggingSystem::resetSinkOfGroup(const std::string &group_name) {
    std::lock_guard guard(mutex_);

    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto &group = it->second;
      // Reset sink to inherit from parent
      setSinkOfGroup(group, {});
      return true;
    }
    return false;
  }

  bool LoggingSystem::setLevelOfGroup(const std::string &group_name,
                                      Level level) {
    std::lock_guard guard(mutex_);
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto &group = it->second;
      // Set new logging level for the group
      setLevelOfGroup(group, level);
      return true;
    }
    return false;
  }

  bool LoggingSystem::resetLevelOfGroup(const std::string &group_name) {
    std::lock_guard guard(mutex_);
    if (auto it = groups_.find(group_name); it != groups_.end()) {
      auto &group = it->second;
      // Reset logging level to inherit from parent
      setLevelOfGroup(group, {});
      return true;
    }
    return false;
  }

  bool LoggingSystem::setGroupOfLogger(const std::string &logger_name,
                                       const std::string &group_name) {
    std::lock_guard guard(mutex_);
    if (auto group = getGroup(group_name)) {
      if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
        if (auto logger = it->second.lock()) {
          // Assign the logger to a new group
          logger->setGroup(std::move(group));
          return true;
        }
        loggers_.erase(it);  // Remove expired logger
      }
    }
    return false;
  }

  bool LoggingSystem::setSinkOfLogger(const std::string &logger_name,
                                      const std::string &sink_name) {
    std::lock_guard guard(mutex_);
    if (auto sink = getSink(sink_name)) {
      if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
        if (auto logger = it->second.lock()) {
          // Assign a new sink to the logger
          logger->setSink(std::move(sink));
          return true;
        }
        loggers_.erase(it);  // Remove expired logger
      }
    }
    return false;
  }

  bool LoggingSystem::resetSinkOfLogger(const std::string &logger_name) {
    std::lock_guard guard(mutex_);
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        // Reset the logger's sink to inherit from its group
        logger->setSinkFromGroup(logger->group());
        return true;
      }
    }
    return false;
  }

  bool LoggingSystem::setLevelOfLogger(const std::string &logger_name,
                                       Level level) {
    std::lock_guard guard(mutex_);
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        // Set a new log level for the logger
        logger->setLevel(level);
        return true;
      }
      loggers_.erase(it);  // Remove expired logger
    }
    return false;
  }

  bool LoggingSystem::resetLevelOfLogger(const std::string &logger_name) {
    std::lock_guard guard(mutex_);
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        // Reset the logger's level to inherit from its group
        logger->setLevelFromGroup(logger->group());
        return true;
      }
      loggers_.erase(it);  // Remove expired logger
    }
    return false;
  }
  void LoggingSystem::callRotateForAllSinks() {
    std::lock_guard guard(mutex_);
    std::ranges::for_each(sinks_, [](const auto &it) {
      const auto &sink = it.second;
      sink->rotate();
    });
  }

}  // namespace soralog
