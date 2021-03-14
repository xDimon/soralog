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

    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) = 0;

    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        Level level) = 0;

    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name) = 0;

    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name, Level level) = 0;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERFACTORY
