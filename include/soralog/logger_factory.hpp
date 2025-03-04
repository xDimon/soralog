/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <soralog/level.hpp>

namespace soralog {

  class Logger;

  /**
   * @class LoggerFactory
   * @brief Abstract factory for creating and managing loggers.
   *
   * This factory provides methods to obtain loggers, creating them
   * if they do not already exist. It supports setting logging levels
   * and output sinks dynamically.
   */
  class LoggerFactory {
   public:
    virtual ~LoggerFactory() = default;

    /**
     * @brief Retrieves or creates a logger with a specified name and group.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group to which the logger belongs.
     * @return Shared pointer to the requested logger.
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) = 0;

    /**
     * @brief Retrieves or creates a logger with a specified name, group,
     *        and overridden logging level.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group to which the logger belongs.
     * @param level Logging level to override.
     * @return Shared pointer to the requested logger.
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        Level level) = 0;

    /**
     * @brief Retrieves or creates a logger with a specified name, group,
     *        and overridden sink.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group to which the logger belongs.
     * @param sink_name Name of the sink to override.
     * @return Shared pointer to the requested logger.
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        std::string sink_name) = 0;

    /**
     * @brief Retrieves or creates a logger with a specified name, group,
     *        overridden sink, and overridden logging level.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group to which the logger belongs.
     * @param sink_name Name of the sink to override.
     * @param level Logging level to override.
     * @return Shared pointer to the requested logger.
     */
    [[nodiscard]] virtual std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        std::string sink_name,
        Level level) = 0;
  };

}  // namespace soralog
