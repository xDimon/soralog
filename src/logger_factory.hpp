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

    [[nodiscard]] Log get(std::string logger_name, std::string group_name,
                          std::optional<std::string> sink_name,
                          std::optional<Level> level);

    [[nodiscard]] inline Log get(std::string logger_name,
                                 std::string group_name,
                                 std::string sink_name) {
      return get(std::move(logger_name), std::move(group_name),
                 std::move(sink_name), {});
    }

    [[nodiscard]] inline Log get(std::string logger_name,
                                 std::string group_name, Level level) {
      return get(std::move(logger_name), std::move(group_name), {}, level);
    }

    [[nodiscard]] inline Log get(std::string logger_name,
                                 std::string group_name) {
      return get(std::move(logger_name), std::move(group_name), {}, {});
    }

   private:
    LoggerSystem &logger_system_;
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERFACTORY
