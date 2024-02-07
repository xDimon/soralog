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
  std::shared_ptr<soralog::Logger> log_;
};
