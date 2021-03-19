/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/group.hpp>

#include <cassert>
#include <stdexcept>

namespace soralog {

  Group::Group(LoggingSystem &logging_system, std::string group_name,
               const std::optional<std::string> &parent_name,
               const std::optional<std::string> &sink_name,
               std::optional<Level> level)
      : system_(logging_system), name_(std::move(group_name)) {
    if (parent_name) {
      parent_group_ = system_.getGroup(*parent_name);
      if (not parent_group_) {
        throw std::invalid_argument("Provided parent group does not exist yet");
      }
      setParentGroup(parent_group_);
    }
    if (sink_name) {
      auto sink = system_.getSink(*sink_name);
      if (not sink) {
        throw std::invalid_argument("Provided sink does not exist yet");
      }
      setSink(std::move(sink));
    }
    if (level) {
      setLevel(*level);
    } else if (not parent_group_) {
      throw std::invalid_argument("Level is not provided for root group");
    }
  }

  // Level

  void Group::resetLevel() {
    if (parent_group_) {
      level_ = parent_group_->level();
      is_level_overridden_ = false;
    }
  }

  void Group::setLevel(Level level) {
    if (parent_group_) {
      is_level_overridden_ = true;
    }
    level_ = level;
  }

  void Group::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    is_level_overridden_ = group != parent_group_;
    level_ = group->level();
  }

  void Group::setLevelFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setLevelFromGroup(group);
    }
  }

  // Sink

  void Group::resetSink() {
    if (parent_group_) {
      sink_ = parent_group_->sink();
      is_sink_overridden_ = false;
    }
  }

  void Group::setSink(std::shared_ptr<const Sink> sink) {
    assert(sink);
    if (parent_group_) {
      is_sink_overridden_ = true;
    }
    sink_ = std::move(sink);
  }

  void Group::setSink(const std::string &sink_name) {
    if (auto sink = system_.getSink(sink_name)) {
      setSink(std::move(sink));
    }
  }

  void Group::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    is_sink_overridden_ = group != parent_group_;
    sink_ = group->sink();
  }

  void Group::setSinkFromGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setSinkFromGroup(group);
    }
  }

  // Parent group

  void Group::unsetParentGroup() {
    parent_group_.reset();
  }

  void Group::setParentGroup(std::shared_ptr<const Group> group) {
    parent_group_ = std::move(group);
    if (parent_group_) {
      if (not is_sink_overridden_) {
        setSinkFromGroup(parent_group_);
      }
      if (not is_level_overridden_) {
        setLevelFromGroup(parent_group_);
      }
    }
  }

  void Group::setParentGroup(const std::string &group_name) {
    if (auto group = system_.getGroup(group_name)) {
      setParentGroup(group);
    }
  }

}  // namespace soralog
