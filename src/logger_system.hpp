/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGERMANAGER
#define SORALOG_LOGGERMANAGER

#include <map>

#include <sink.hpp>
#include <sink/sink_to_console.hpp>
#include <sink/sink_to_file.hpp>

namespace soralog {

  class Group;

  class LoggerSystem final {
   public:
    LoggerSystem() = default;
    LoggerSystem(const LoggerSystem &) = delete;
    LoggerSystem &operator=(LoggerSystem const &) = delete;
    ~LoggerSystem() = default;
    LoggerSystem(LoggerSystem &&tmp) noexcept = delete;
    LoggerSystem &operator=(LoggerSystem &&tmp) noexcept = delete;

    [[nodiscard]] std::shared_ptr<Sink> getLogger(const std::string &name);

    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &name);

    [[nodiscard]] std::shared_ptr<Group> getGroup(const std::string &name);

    template <typename SinkType, typename... Args>
    void makeSink(Args &&... args) {
      auto sink = std::make_shared<SinkType>(std::forward<Args>(args)...);
      sinks_[sink->name()] = std::move(sink);
    }

    void makeGroup(std::string name, const std::optional<std::string> &parent,
                   const std::optional<std::string> &sink,
                   const std::optional<Level> &level);

   private:
    std::map<std::string, std::shared_ptr<Sink>> sinks_;
    std::map<std::string, std::shared_ptr<Group>> groups_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERMANAGER
