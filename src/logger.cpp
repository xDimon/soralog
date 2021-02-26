/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logger.hpp"
namespace soralog {

  Logger::Logger(soralog::LoggerSystem &system, std::string name,
                 const std::string &group_name)
      : system_(system),
        name_(std::move(name)),
        group_(system_.getGroup(group_name)) {
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

  void Logger::setLevelFromGroup(const std::shared_ptr<Group> &group) {
    level_is_overriden_ = group != group_;
    level_ = group->level();
  }

  void Logger::setLevelFromGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setLevelFromGroup(group);
  }

  // Sink

  void Logger::resetSink() {
    sink_ = system_.getSink(group_->sink());
    sink_is_overriden_ = false;
  }

  void Logger::setSink(const std::string &sink_name) {
    if (auto sink = system_.getSink(sink_name)) {
      sink_is_overriden_ = true;
      sink_ = std::move(sink);
    }
  }

  void Logger::setSinkFromGroup(const std::shared_ptr<Group> &group) {
    if (auto sink = system_.getSink(group->sink())) {
      sink_is_overriden_ = group != group_;
      sink_ = std::move(sink);
    }
  }

  void Logger::setSinkFromGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setSinkFromGroup(group);
  }

  // Parent group

  void Logger::setGroup(std::shared_ptr<Group> group) {
    group_ = std::move(group);
    if (group_) {
      setSinkFromGroup(group_);
      setLevelFromGroup(group_);
    } else {
      sink_is_overriden_ = false;
      level_is_overriden_ = false;
    }
  }

  void Logger::setGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setGroup(group);
  }

}  // namespace soralog
