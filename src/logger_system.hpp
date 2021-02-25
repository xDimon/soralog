/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGERMANAGER
#define SORALOG_LOGGERMANAGER

#include <map>

#include <boost/assert.hpp>

#include <sink.hpp>
#include <sink/sink_to_console.hpp>
#include <sink/sink_to_file.hpp>

namespace soralog {

  class LoggerSystem final {
   public:
    LoggerSystem(const LoggerSystem &) = delete;
    LoggerSystem &operator=(LoggerSystem const &) = delete;
    ~LoggerSystem() = default;
    LoggerSystem(LoggerSystem &&tmp) noexcept = delete;
    LoggerSystem &operator=(LoggerSystem &&tmp) noexcept = delete;

    LoggerSystem();

    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &sink_name);

   private:
    std::map<std::string, std::shared_ptr<Sink>> sinks_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERMANAGER
