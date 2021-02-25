/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGERFACTORY
#define SORALOG_LOGGERFACTORY

#include <logger.hpp>
#include <logger_system.hpp>
#include <string_view>
#include <unordered_map>

namespace soralog {

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
                          Level level);

   private:
    LoggerSystem &logger_system_;
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERFACTORY
