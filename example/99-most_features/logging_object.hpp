/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <soralog/logger.hpp>
#include <soralog/logger_factory.hpp>

/**
 * @file logging_object.hpp
 * @brief Example: integrating a per-class logger created via LoggerFactory.
 *
 * The purpose of this file is to demonstrate a common integration pattern:
 * - A class stores a `std::shared_ptr<soralog::Logger>` as a member.
 * - The logger is obtained from a `soralog::LoggerFactory` in the constructor.
 * - Member functions emit log messages without needing to know sink/group
 *   details (those are configured in the logging system).
 */

/**
 * @class LoggingObject
 * @brief Example class demonstrating how to integrate soralog into a component.
 *
 * This class encapsulates logging functionality using a logger instance
 * created from a `LoggerFactory`. The logger can be used to emit messages
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
   * @brief Constructs the object and initializes its logger.
   * @param logger_factory Reference to the factory used to obtain a configured
   * logger.
   */
  explicit LoggingObject(soralog::LoggerFactory &logger_factory);

  /**
   * @brief Example method that logs a message.
   *
   * Shows how a component uses its member logger in a method.
   */
  void method() const;

 private:
  std::shared_ptr<soralog::Logger> log_;  ///< Per-instance logger.
};
