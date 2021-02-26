/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logger_system.hpp>

#include <group.hpp>

namespace soralog {

  LoggerSystem::LoggerSystem() {
    auto default_sink = std::make_shared<SinkToConsole>("console");
    sinks_["default"] = default_sink;
    sinks_[default_sink->name()] = default_sink;

    auto default_group =
        std::make_shared<Group>(*this, "default", "console", Level::INFO);
    groups_["default"] = default_group;
    groups_[default_sink->name()] = default_group;

    // Custom

    auto file_sink = std::make_shared<SinkToFile>("file", "soralog.log");
    sinks_[file_sink->name()] = file_sink;
  };

  [[nodiscard]] std::shared_ptr<Sink> LoggerSystem::getSink(
      const std::string &sink_name) {
    auto it = sinks_.find(sink_name);
    if (it == sinks_.end()) {
      it = sinks_.find("default");
      assert(it != sinks_.end());
    }
    return it->second;
  }

  [[nodiscard]] std::shared_ptr<Group> LoggerSystem::getGroup(
      const std::string &group_name) {
    auto it = groups_.find(group_name);
    if (it == groups_.end()) {
      it = groups_.find("default");
      assert(it != groups_.end());
    }
    return it->second;
  }

}  // namespace soralog
