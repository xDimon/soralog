/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/logger.hpp>

#include <soralog/logger_system.hpp>
#include <soralog/group.hpp>

namespace soralog {

  Logger::Logger(soralog::LoggerSystem &system, std::string logger_name,
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
    level_is_overriden_ = true;
    level_ = level;
  }

  void Logger::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    level_is_overriden_ = group != group_;
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
    sink_is_overriden_ = false;
  }

  void Logger::setSink(const std::string &sink_name) {
    if (auto sink = system_.getSink(sink_name)) {
      setSink(std::move(sink));
    }
  }

  void Logger::setSink(std::shared_ptr<Sink> sink) {
    assert(sink);
    sink_is_overriden_ = true;
    sink_ = std::move(sink);
  }

  void Logger::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    if (auto sink = std::const_pointer_cast<Sink>(group->sink())) {
      sink_is_overriden_ = group != group_;
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
    if (group) {
      group_ = std::move(group);
      setSinkFromGroup(group_);
      setLevelFromGroup(group_);
    } else {
      sink_is_overriden_ = false;
      level_is_overriden_ = false;
    }
  }

  void Logger::setGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setGroup(group);
    }
  }

}  // namespace soralog
