/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "group.hpp"

namespace soralog {

  Group::Group(LoggerSystem &logger_system, std::string group_name,
               const std::string &parent_group, const std::string &sink_name,
               Level level)
      : system_(logger_system), name_(std::move(group_name)) {
    parent_group_ = system_.getGroup(parent_group);
    if (parent_group_) {
      assert(parent_group_);
      setParentGroup(parent_group_);
    }
    setSink(sink_name);
    setLevel(level);
  }

  Group::Group(LoggerSystem &logger_system, std::string group_name,
               const std::string &sink_name, Level level)
      : system_(logger_system), name_(std::move(group_name)) {
    setSink(sink_name);
    setLevel(level);
  }

  void Group::resetLevel() {
    if (parent_group_) {
      level_ = parent_group_->level();
      level_is_overriden_ = false;
    }
  }

  void Group::setLevel(Level level) {
    level_is_overriden_ = true;
    level_ = level;
  }

  void Group::setLevelFromGroup(const std::shared_ptr<Group> &group) {
    level_is_overriden_ = group != parent_group_;
    level_ = group->level();
  }

  void Group::setLevelFromGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setLevelFromGroup(group);
  }

  void Group::resetSink() {
    sink_ = parent_group_->sink();
    sink_is_overriden_ = false;
  }

  void Group::setSink(std::string sink) {
    sink_is_overriden_ = true;
    sink_ = std::move(sink);
  }

  void Group::setSinkFromGroup(const std::shared_ptr<Group> &group) {
    sink_is_overriden_ = group != parent_group_;
    sink_ = group->sink();
  }

  void Group::setSinkFromGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setSinkFromGroup(group);
  }

  void Group::setParentGroup(std::shared_ptr<Group> group) {
    parent_group_ = std::move(group);
    if (parent_group_) {
      setSinkFromGroup(parent_group_);
      setLevelFromGroup(parent_group_);
    }
  }

  void Group::setParentGroup(const std::string &group_name) {
    auto group = system_.getGroup(group_name);
    setParentGroup(group);
  }

}  // namespace soralog
