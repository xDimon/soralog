/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iostream>
#include <thread>

#include <soralog/level.hpp>
#include <soralog/logger.hpp>
#include <soralog/logger_factory.hpp>

/**
 * @class LoggingObject
 * @brief Example class demonstrating how to integrate logging using soralog.
 *
 * This class encapsulates logging functionality using a logger instance
 * created from a `LoggerFactory`. The logger is used to produce messages
 * at different levels.
 */
class LoggingObject final {
 public:
  LoggingObject() = delete;
  LoggingObject(LoggingObject &&) noexcept = delete;
  LoggingObject(const LoggingObject &) = delete;
  ~LoggingObject() = default;
  LoggingObject &operator=(LoggingObject &&) noexcept = delete;
  LoggingObject &operator=(const LoggingObject &) = delete;

  /**
   * @brief Constructs the `LoggingObject` and initializes the logger.
   * @param logger_factory Reference to the logger factory for obtaining a
   * logger.
   */
  explicit LoggingObject(soralog::LoggerFactory &logger_factory);

  /**
   * @brief Example method that logs a message.
   *
   * Demonstrates how to use the logger instance within a class.
   */
  void method() const;

 private:
  std::shared_ptr<soralog::Logger> log_;  ///< Logger instance for this object.
};
