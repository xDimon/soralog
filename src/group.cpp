/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/group.hpp>

#include <cassert>
#include <stdexcept>

namespace soralog {

  Group::Group(LoggingSystem &logging_system,
               std::string group_name,
               const std::optional<std::string> &parent_name,
               const std::optional<std::string> &sink_name,
               std::optional<Level> level)
      : system_(logging_system), name_(std::move(group_name)) {
    // If a parent group name is provided, retrieve and assign it
    if (parent_name) {
      parent_group_ = system_.getGroup(*parent_name);
      if (not parent_group_) {
        throw std::invalid_argument("Provided parent group does not exist yet");
      }
      setParentGroup(parent_group_);
    }

    // If a sink name is provided, retrieve and assign it
    if (sink_name) {
      auto sink = system_.getSink(*sink_name);
      if (not sink) {
        throw std::invalid_argument("Provided sink does not exist yet");
      }
      setSink(std::move(sink));
    }

    // If a logging level is provided, set it; otherwise, require a parent group
    if (level) {
      setLevel(*level);
    } else if (not parent_group_) {
      throw std::invalid_argument("Level is not provided for root group");
    }
  }

  // Level Management

  void Group::resetLevel() {
    // Restore level from the parent group if available
    if (parent_group_) {
      level_ = parent_group_->level();
      is_level_overridden_ = false;
    }
  }

  void Group::setLevel(Level level) {
    // Mark the level as overridden if there is a parent group
    if (parent_group_) {
      is_level_overridden_ = true;
    }
    level_ = level;
  }

  void Group::setLevelFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    // Only override if the provided group differs from the current parent
    is_level_overridden_ = group != parent_group_;
    level_ = group->level();
  }

  void Group::setLevelFromGroup(const std::string &group_name) {
    // Attempt to retrieve the group and apply its level
    if (auto group = system_.getGroup(group_name)) {
      setLevelFromGroup(group);
    }
  }

  // Sink Management

  void Group::resetSink() {
    // Restore sink from the parent group if available
    if (parent_group_) {
      sink_ = parent_group_->sink();
      is_sink_overridden_ = false;
    }
  }

  void Group::setSink(std::shared_ptr<const Sink> sink) {
    assert(sink);
    // Mark the sink as overridden if there is a parent group
    if (parent_group_) {
      is_sink_overridden_ = true;
    }
    sink_ = std::move(sink);
  }

  void Group::setSink(const std::string &sink_name) {
    // Attempt to retrieve the sink and apply it
    if (auto sink = system_.getSink(sink_name)) {
      setSink(std::move(sink));
    }
  }

  void Group::setSinkFromGroup(const std::shared_ptr<const Group> &group) {
    assert(group);
    // Only override if the provided group differs from the current parent
    is_sink_overridden_ = group != parent_group_;
    sink_ = group->sink();
  }

  void Group::setSinkFromGroup(const std::string &group_name) {
    // Attempt to retrieve the group and apply its sink
    if (auto group = system_.getGroup(group_name)) {
      setSinkFromGroup(group);
    }
  }

  // Parent Group Management

  void Group::unsetParentGroup() {
    // Remove the reference to the parent group
    parent_group_.reset();
  }

  void Group::setParentGroup(std::shared_ptr<const Group> group) {
    parent_group_ = std::move(group);

    // If a parent group is set, inherit properties if they are not overridden
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
    // Attempt to retrieve the group and set it as the parent
    if (auto group = system_.getGroup(group_name)) {
      setParentGroup(group);
    }
  }

}  // namespace soralog
