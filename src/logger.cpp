/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/logger.hpp>

#include <soralog/group.hpp>
#include <soralog/logging_system.hpp>

namespace soralog {

  Logger::Logger(soralog::LoggingSystem &system,
                 std::string logger_name,
                 std::shared_ptr<const Group> group)
      : system_(system),
        name_(std::move(logger_name)),
        group_(std::move(group)) {
    assert(group_);             // Ensure that a valid group is provided
    setSinkFromGroup(group_);   // Inherit sink from the assigned group
    setLevelFromGroup(group_);  // Inherit log level from the assigned group
  }

  // Level Management

  void Logger::resetLevel() {
    // Reset the log level to the group's level (removing any overrides)
    setLevelFromGroup(group_);
  }

  void Logger::setLevel(Level level) {
    // Override the inherited log level with a custom one
    is_level_overridden_ = true;
    level_ = level;
  }

  void Logger::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    // Check if the level is inherited or explicitly overridden
    is_level_overridden_ = group != group_;
    level_ = group->level();
  }

  void Logger::setLevelFromGroup(const std::string &group_name) {
    // Attempt to find the group by name and apply its log level
    if (auto group = system_.getGroup(group_name)) {
      setLevelFromGroup(group);
    }
  }

  // Sink Management

  void Logger::resetSink() {
    // Reset the sink to the one assigned to the group (removing overrides)
    sink_ = std::const_pointer_cast<Sink>(group_->sink());
    is_sink_overridden_ = false;
  }

  void Logger::setSink(const std::string &sink_name) {
    // Attempt to find the sink by name and apply it to this logger
    if (auto sink = system_.getSink(sink_name)) {
      setSink(std::move(sink));
    }
  }

  void Logger::setSink(std::shared_ptr<Sink> sink) {
    assert(sink);
    // Assign the new sink and mark it as an override
    is_sink_overridden_ = true;
    sink_ = std::move(sink);
  }

  void Logger::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    // Only override if the group is different from the current one
    if (auto sink = std::const_pointer_cast<Sink>(group->sink())) {
      is_sink_overridden_ = group != group_;
      sink_ = std::move(sink);
    }
  }

  void Logger::setSinkFromGroup(const std::string &group_name) {
    // Attempt to find the group by name and inherit its sink
    if (auto group = system_.getGroup(group_name)) {
      setSinkFromGroup(group);
    }
  }

  // Group Management

  void Logger::setGroup(std::shared_ptr<const Group> group) {
    assert(group);
    // Change the logger's group and update inherited properties if needed
    group_ = std::move(group);
    if (not is_sink_overridden_) {
      setSinkFromGroup(group_);
    }
    if (not is_level_overridden_) {
      setLevelFromGroup(group_);
    }
  }

  void Logger::setGroup(const std::string &group_name) {
    // Attempt to find the group by name and assign it to the logger
    if (auto group = system_.getGroup(group_name)) {
      setGroup(group);
    }
  }

}  // namespace soralog
