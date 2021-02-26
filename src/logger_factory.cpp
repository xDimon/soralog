/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logger_factory.hpp"

namespace soralog {

  LoggerFactory::LoggerFactory(LoggerSystem &logger_system)
      : logger_system_(logger_system) {}

  Log LoggerFactory::get(std::string logger_name, std::string group_name,
                         std::optional<std::string> sink_name,
                         std::optional<Level> level) {
    if (auto it = loggers_.find(logger_name); it != loggers_.end()) {
      if (auto logger = it->second.lock()) {
        return std::move(logger);
      }
    }

    auto logger = std::make_shared<Logger>(
        logger_system_, std::move(logger_name), std::move(group_name));

    if (sink_name.has_value()) {
      logger->setSink(std::move(sink_name).value());
    }

    if (level.has_value()) {
      logger->setLevel(level.value());
    }

    loggers_[logger->name()] = logger;
    return logger;
  }

}  // namespace soralog
