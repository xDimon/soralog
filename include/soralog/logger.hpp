/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOG
#define SORALOG_LOG

#include <memory>
#include <string>

#include <soralog/level.hpp>
#include <soralog/sink.hpp>

namespace soralog {

  class LoggingSystem;
  class Group;

  class Logger final {
   public:
    Logger() = delete;
    Logger(Logger &&) noexcept = delete;
    Logger(const Logger &) = delete;
    ~Logger() = default;
    Logger &operator=(Logger &&) noexcept = delete;
    Logger &operator=(Logger const &) = delete;

    Logger(LoggingSystem &system, std::string logger_name,
           std::shared_ptr<const Group> group);

   private:
    template <typename... Args>
    void push(Level level, std::string_view format, const Args &... args) {
      if (level_ >= level) {
        sink_->push(name_, level, format, args...);
      }
    }

   public:
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    template <typename... Args>
    void log(Level level, std::string_view format, const Args &... args) {
      push(level, format, args...);
    }

    template <typename Arg>
    void trace(Level level, const Arg &arg) {
      push(level, "{}", arg);
    }

    template <typename... Args>
    void trace(std::string_view format, const Args &... args) {
      push(Level::TRACE, format, args...);
    }

    template <typename Arg>
    void trace(const Arg &arg) {
      push(Level::TRACE, "{}", arg);
    }

    template <typename... Args>
    void debug(std::string_view format, const Args &... args) {
      push(Level::DEBUG, format, args...);
    }

    template <typename Arg>
    void debug(const Arg &arg) {
      push(Level::DEBUG, "{}", arg);
    }

    template <typename... Args>
    void verbose(std::string_view format, const Args &... args) {
      push(Level::VERBOSE, format, args...);
    }

    template <typename Arg>
    void verbose(const Arg &arg) {
      push(Level::VERBOSE, "{}", arg);
    }

    template <typename... Args>
    void info(std::string_view format, const Args &... args) {
      push(Level::INFO, format, args...);
    }

    template <typename Arg>
    void info(const Arg &arg) {
      push(Level::INFO, "{}", arg);
    }

    template <typename... Args>
    void warn(std::string_view format, const Args &... args) {
      push(Level::WARN, format, args...);
    }

    template <typename Arg>
    void warn(const Arg &arg) {
      push(Level::WARN, "{}", arg);
    }

    template <typename... Args>
    void error(std::string_view format, const Args &... args) {
      push(Level::ERROR, format, args...);
    }

    template <typename Arg>
    void error(const Arg &arg) {
      push(Level::ERROR, "{}", arg);
    }

    template <typename... Args>
    void critical(std::string_view format, const Args &... args) {
      push(Level::CRITICAL, format, args...);
    }

    template <typename Arg>
    void critical(const Arg &arg) {
      push(Level::CRITICAL, "{}", arg);
    }

    void flush() const {
      sink_->flush();
    }

    // Level

    [[nodiscard]] Level level() const noexcept {
      return level_;
    }
    [[nodiscard]] bool hasLevelOverriden() const noexcept {
      return has_level_overriden_;
    }

    void resetLevel();
    void setLevel(Level level);
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }
    [[nodiscard]] bool hasSinkOverriden() const noexcept {
      return has_sink_overriden_;
    }

    void resetSink();
    void setSink(const std::string &sink_name);
    void setSink(std::shared_ptr<Sink> sink);
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    [[nodiscard]] std::shared_ptr<const Group> group() const noexcept {
      return group_;
    }

    void setGroup(std::shared_ptr<const Group> group);
    void setGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;

    const std::string name_;
    std::shared_ptr<const Group> group_;

    std::shared_ptr<Sink> sink_;
    bool has_sink_overriden_{};

    Level level_{};
    bool has_level_overriden_{};
  };

  using Log = std::shared_ptr<Logger>;

}  // namespace soralog

#endif  // SORALOG_LOG
