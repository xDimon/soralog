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

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name} and group {@param group_name}
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) = 0;

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and level
     * overridden to {@param level}
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        Level level) = 0;

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and sink
     * overridden to sink with name {@param sink_name}
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name) = 0;

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and sink
     * andd level overridden to {@param sink_name} and {@param level}
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name, Level level) = 0;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERFACTORY
