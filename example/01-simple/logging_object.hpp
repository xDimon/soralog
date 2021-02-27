/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_EXAMPLE_LOGGINGOBJECT
#define SORALOG_EXAMPLE_LOGGINGOBJECT

#include <iostream>
#include <log_levels.hpp>
#include <logger.hpp>
#include <logger_factory.hpp>
#include <thread>

class LoggingObject final {
 public:
  LoggingObject() = delete;
  LoggingObject(LoggingObject &&) noexcept = delete;
  LoggingObject(const LoggingObject &) = delete;
  ~LoggingObject() = default;
  LoggingObject &operator=(LoggingObject &&) noexcept = delete;
  LoggingObject &operator=(LoggingObject const &) = delete;

  explicit LoggingObject(soralog::LoggerFactory &logger_factory);

  void method() const;

 private:
  soralog::Log log_;
};

#endif  // SORALOG_EXAMPLE_LOGGINGOBJECT
