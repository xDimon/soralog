/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/group.hpp>

namespace soralog {

  Group::Group(LoggingSystem &logging_system, std::string group_name,
               const std::optional<std::string> &parent_name,
               const std::optional<std::string> &sink_name,
               std::optional<Level> level)
      : system_(logging_system), name_(std::move(group_name)) {
    if (parent_name) {
      parent_group_ = system_.getGroup(*parent_name);
      if (parent_group_) {
        assert(parent_group_);
        setParentGroup(parent_group_);
      }
    }
    if (sink_name) {
      auto sink = system_.getSink(*sink_name);
      setSink(std::move(sink));
    }
    if (level) {
      setLevel(*level);
    }
  }

  // Level

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

  void Group::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    level_is_overriden_ = group != parent_group_;
    level_ = group->level();
  }

  void Group::setLevelFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setLevelFromGroup(group);
    }
  }

  // Sink

  void Group::resetSink() {
    sink_ = parent_group_->sink();
    sink_is_overriden_ = false;
  }

  void Group::setSink(std::shared_ptr<const Sink> sink) {
    assert(sink);
    sink_is_overriden_ = true;
    sink_ = std::move(sink);
  }

  void Group::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    sink_is_overriden_ = group != parent_group_;
    sink_ = group->sink();
  }

  void Group::setSinkFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setSinkFromGroup(group);
    }
  }

  // Parent group

  void Group::setParentGroup(std::shared_ptr<const Group> group) {
    parent_group_ = std::move(group);
    if (parent_group_) {
      setSinkFromGroup(parent_group_);
      setLevelFromGroup(parent_group_);
    } else {
      sink_is_overriden_ = false;
      level_is_overriden_ = false;
    }
  }

  void Group::setParentGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setParentGroup(group);
    }
  }

}  // namespace soralog
