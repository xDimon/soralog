/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_LOGGERMANAGER
#define XLOG_LOGGERMANAGER

#include <map>

#include <boost/assert.hpp>

#include <sink.hpp>
#include <sink/sink_to_console.hpp>
#include <sink/sink_to_file.hpp>

namespace xlog {

  class LoggerSystem final {
   public:
    LoggerSystem(const LoggerSystem &) = delete;
    LoggerSystem &operator=(LoggerSystem const &) = delete;
    ~LoggerSystem() = default;
    LoggerSystem(LoggerSystem &&tmp) noexcept = delete;
    LoggerSystem &operator=(LoggerSystem &&tmp) noexcept = delete;

    LoggerSystem() {
      auto default_sink = std::make_shared<SinkToConsole>("console");
      sinks_["default"] = default_sink;
      sinks_[default_sink->name()] = default_sink;

      auto file_sink = std::make_shared<SinkToFile>("file", "xlog.log");
      sinks_[file_sink->name()] = file_sink;
    };

    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &sink_name) {
      auto it = sinks_.find(sink_name);
      if (it == sinks_.end()) {
        it = sinks_.find("default");
        BOOST_ASSERT(it != sinks_.end());
      }
      return it->second;
    }

    //    std::mutex _mutex;
    //
    //    std::map<std::string, std::shared_ptr<Sink>> _sinks;
    //    std::map<std::string, std::tuple<std::shared_ptr<Sink>, Log::Detail>>
    //        _loggers;
    //
    //   public:
    //    static void init(const std::shared_ptr<Config> &configs);
    //
    //    static const std::tuple<std::shared_ptr<Sink>, Log::Detail>
    //        &getSinkAndLevel(const std::string &loggerName);
    //    static std::shared_ptr<Sink> getSink(const std::string &sinkName);
    //
    //    static void finalFlush();
    //
    //    static void rotate();

   private:
    std::map<std::string, std::shared_ptr<Sink>> sinks_;
  };

}  // namespace xlog

#endif  // XLOG_LOGGERMANAGER
