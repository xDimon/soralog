/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logger_factory.hpp"

namespace soralog {

  LoggerFactory::LoggerFactory(LoggerSystem &logger_system)
      : logger_system_(logger_system) {}

  Log LoggerFactory::get(std::string logger_name, std::string_view sink_name,
                         Level level) {
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        return std::move(logger);
      }
    }

    auto sink = logger_system_.getSink(std::string(sink_name));

    auto logger = std::make_shared<Logger>(std::move(logger_name),
                                           std::move(sink), level);

    loggers_[logger->name()] = logger;
    return logger;
  }

}  // namespace soralog
