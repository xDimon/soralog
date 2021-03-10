/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGERFACTORY
#define SORALOG_LOGGERFACTORY

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <soralog/level.hpp>

namespace soralog {

  class Logger;

  class LoggerFactory {
   public:
    virtual ~LoggerFactory() = default;

   protected:
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        const std::optional<std::string> &sink_name,
        const std::optional<Level> &level) = 0;

   public:
    [[nodiscard]] inline std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name) {
      return getLogger(std::move(logger_name), group_name,
                       {std::move(sink_name)}, {});
    }

    [[nodiscard]] inline std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name, Level level) {
      return getLogger(std::move(logger_name), group_name, {}, {level});
    }

    [[nodiscard]] inline std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) {
      return getLogger(std::move(logger_name), group_name, {}, {});
    }
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERFACTORY
