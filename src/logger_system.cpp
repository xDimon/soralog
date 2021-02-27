/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logger_system.hpp>

#include <group.hpp>

namespace soralog {

  void LoggerSystem::makeGroup(std::string name,
                               const std::optional<std::string> &parent,
                               const std::optional<std::string> &sink,
                               const std::optional<Level> &level) {
    auto group =
        std::make_shared<Group>(*this, std::move(name), parent, sink, level);
    groups_[group->name()] = std::move(group);
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

}  // namespace soralog
