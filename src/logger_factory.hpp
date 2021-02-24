/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_LOGGERFACTORY
#define XLOG_LOGGERFACTORY

#include <logger.hpp>
#include <logger_system.hpp>
#include <unordered_map>
#include <string_view>

namespace xlog {

  class LoggerFactory final {
   public:
    LoggerFactory() = delete;
    LoggerFactory(LoggerFactory &&) noexcept = delete;
    LoggerFactory(const LoggerFactory &) = delete;
    virtual ~LoggerFactory() = default;
    LoggerFactory &operator=(LoggerFactory &&) noexcept = delete;
    LoggerFactory &operator=(LoggerFactory const &) = delete;

    LoggerFactory(LoggerSystem &logger_system);

    [[nodiscard]] Log get(std::string logger_name, std::string_view sink_name,
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

   private:
    LoggerSystem &logger_system_;
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
  };

}  // namespace xlog

#endif  // XLOG_LOGGERFACTORY
