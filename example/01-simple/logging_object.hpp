/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_EXAMPLE_LOGGINGOBJECT
#define XLOG_EXAMPLE_LOGGINGOBJECT

#include <log_levels.hpp>
#include <logger.hpp>
#include <logger_factory.hpp>
#include <thread>
#include <iostream>

class LoggingObject final {
 public:
  LoggingObject() = delete;
  LoggingObject(LoggingObject &&) noexcept = delete;
  LoggingObject(const LoggingObject &) = delete;
  ~LoggingObject() = default;
  LoggingObject &operator=(LoggingObject &&) noexcept = delete;
  LoggingObject &operator=(LoggingObject const &) = delete;

  LoggingObject(xlog::LoggerFactory &logger_factory);

  void method() const;

 private:
  xlog::Log log_;
};

#endif  // XLOG_EXAMPLE_LOGGINGOBJECT
