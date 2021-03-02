/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOG
#define SORALOG_LOG

#include <memory>
#include <string>

#include <soralog/sink.hpp>
#include <soralog/level.hpp>

namespace soralog {

  class LoggerSystem;
  class Group;

  class Logger final {
   public:
    Logger() = delete;
    Logger(Logger &&) noexcept = delete;
    Logger(const Logger &) = delete;
    ~Logger() = default;
    Logger &operator=(Logger &&) noexcept = delete;
    Logger &operator=(Logger const &) = delete;

    Logger(LoggerSystem &system, std::string logger_name,
           std::shared_ptr<const Group> group);

   private:
    template <typename... Args>
    void push(Level level, std::string_view format, Args... args) {
      if (level_ >= level) {
        sink_->push(name_, level, format, std::forward<Args>(args)...);
      }
    }

   public:
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    template <typename... Args>
    void trace(std::string_view format, Args &&... args) {
      push(Level::TRACE, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void trace(Arg &&msg) {
      push(Level::TRACE, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void debug(std::string_view format, Args &&... args) {
      push(Level::DEBUG, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void debug(Arg &&msg) {
      push(Level::DEBUG, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void verbose(std::string_view format, Args &&... args) {
      push(Level::VERBOSE, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void verbose(Arg &&msg) {
      push(Level::VERBOSE, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void info(std::string_view format, Args &&... args) {
      push(Level::INFO, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void info(Arg &&msg) {
      push(Level::INFO, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void warn(std::string_view format, Args &&... args) {
      push(Level::WARN, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void warn(Arg &&msg) {
      push(Level::WARN, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void error(std::string_view format, Args &&... args) {
      push(Level::ERROR, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void error(Arg &&msg) {
      push(Level::ERROR, "{}", std::forward<Arg>(msg));
    }

    template <typename... Args>
    void critical(std::string_view format, Args &&... args) {
      push(Level::CRITICAL, format, std::forward<Args>(args)...);
    }

    template <class Arg>
    void critical(Arg &&msg) {
      push(Level::CRITICAL, "{}", std::forward<Arg>(msg));
    }

    void flush() const {
      sink_->flush();
    }

    // Level

    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    void resetLevel();
    void setLevel(Level level);
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
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

    void setGroup(const std::string &group_name);
    void setGroup(std::shared_ptr<const Group> group);

   private:
    LoggerSystem &system_;

    const std::string name_;
    std::shared_ptr<const Group> group_;

    std::shared_ptr<Sink> sink_;
    bool sink_is_overriden_{};

    Level level_{};
    bool level_is_overriden_{};
  };

  using Log = std::shared_ptr<Logger>;

}  // namespace soralog

#endif  // SORALOG_LOG
