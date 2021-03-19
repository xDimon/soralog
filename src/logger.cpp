/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/logger.hpp>

#include <soralog/group.hpp>
#include <soralog/logging_system.hpp>

namespace soralog {

  Logger::Logger(soralog::LoggingSystem &system, std::string logger_name,
                 std::shared_ptr<const Group> group)
      : system_(system),
        name_(std::move(logger_name)),
        group_(std::move(group)) {
    assert(group_);
    setSinkFromGroup(group_);
    setLevelFromGroup(group_);
  }

  // Level

  void Logger::resetLevel() {
    setLevelFromGroup(group_);
  }

  void Logger::setLevel(Level level) {
    is_level_overridden_ = true;
    level_ = level;
  }

  void Logger::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    is_level_overridden_ = group != group_;
    level_ = group->level();
  }

  void Logger::setLevelFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setLevelFromGroup(group);
    }
  }

  // Sink

  void Logger::resetSink() {
    sink_ = std::const_pointer_cast<Sink>(group_->sink());
    is_sink_overridden_ = false;
  }

  void Logger::setSink(const std::string &sink_name) {
    if (auto sink = system_.getSink(sink_name)) {
      setSink(std::move(sink));
    }
  }

  void Logger::setSink(std::shared_ptr<Sink> sink) {
    assert(sink);
    is_sink_overridden_ = true;
    sink_ = std::move(sink);
  }

  void Logger::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    if (auto sink = std::const_pointer_cast<Sink>(group->sink())) {
      is_sink_overridden_ = group != group_;
      sink_ = std::move(sink);
    }
  }

  void Logger::setSinkFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setSinkFromGroup(group);
    }
  }

  // Group

  void Logger::setGroup(std::shared_ptr<const Group> group) {
    assert(group);
    group_ = std::move(group);
    if (not is_sink_overridden_) {
      setSinkFromGroup(group_);
    }
    if (not is_level_overridden_) {
      setLevelFromGroup(group_);
    }
  }

  void Logger::setGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setGroup(group);
    }
  }

}  // namespace soralog
