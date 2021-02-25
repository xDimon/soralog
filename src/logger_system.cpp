/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logger_system.hpp"

namespace soralog {

  LoggerSystem::LoggerSystem() {
    auto default_sink = std::make_shared<SinkToConsole>("console");
    sinks_["default"] = default_sink;
    sinks_[default_sink->name()] = default_sink;

    auto file_sink = std::make_shared<SinkToFile>("file", "soralog.log");
    sinks_[file_sink->name()] = file_sink;
  };

  [[nodiscard]] std::shared_ptr<Sink> LoggerSystem::getSink(
      const std::string &sink_name) {
    auto it = sinks_.find(sink_name);
    if (it == sinks_.end()) {
      it = sinks_.find("default");
      BOOST_ASSERT(it != sinks_.end());
    }
    return it->second;
  }

}  // namespace soralog
