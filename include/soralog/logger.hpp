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

  /**
   * @class Logger
   * @brief Entity for filtering events by level and pushing their data to sink
   */
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
    /**
     * Checks if {@param level} of event is enough to logging and push logger
     * name and event's data ({@param format} and {@param args}) to sink
     */
    template <typename... Args>
    void push(Level level, std::string_view format, const Args &... args) {
      if (level_ >= level) {
        sink_->push(name_, level, format, args...);
      }
    }

   public:
    /**
     * @returns name of logger
     */
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    /**
     * Logs event ({@param format} and {@param args})
     * with provoded {@param level}
     */
    template <typename... Args>
    void log(Level level, std::string_view format, const Args &... args) {
      push(level, format, args...);
    }

    /**
     * Logs event ({@param format} and {@param args}) with trace level
     */
    template <typename... Args>
    void trace(std::string_view format, const Args &... args) {
      push(Level::TRACE, format, args...);
    }

    /**
     * Logs value {@param arg} as event with trace level
     */
    template <typename Arg>
    void trace(const Arg &arg) {
      push(Level::TRACE, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with debug level
     */
    template <typename... Args>
    void debug(std::string_view format, const Args &... args) {
      push(Level::DEBUG, format, args...);
    }

    /**
     * Logs value {@param arg} as event with debug level
     */
    template <typename Arg>
    void debug(const Arg &arg) {
      push(Level::DEBUG, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with verbose level
     */
    template <typename... Args>
    void verbose(std::string_view format, const Args &... args) {
      push(Level::VERBOSE, format, args...);
    }

    /**
     * Logs value {@param arg} as event with verbose level
     */
    template <typename Arg>
    void verbose(const Arg &arg) {
      push(Level::VERBOSE, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with info level
     */
    template <typename... Args>
    void info(std::string_view format, const Args &... args) {
      push(Level::INFO, format, args...);
    }

    /**
     * Logs value {@param arg} as event with info level
     */
    template <typename Arg>
    void info(const Arg &arg) {
      push(Level::INFO, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with warning level
     */
    template <typename... Args>
    void warn(std::string_view format, const Args &... args) {
      push(Level::WARN, format, args...);
    }

    /**
     * Logs value {@param arg} as event with warning level
     */
    template <typename Arg>
    void warn(const Arg &arg) {
      push(Level::WARN, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with error level
     */
    template <typename... Args>
    void error(std::string_view format, const Args &... args) {
      push(Level::ERROR, format, args...);
    }

    /**
     * Logs value {@param arg} as event with error level
     */
    template <typename Arg>
    void error(const Arg &arg) {
      push(Level::ERROR, "{}", arg);
    }

    /**
     * Logs event ({@param format} and {@param args}) with level of critical
     * situation
     */
    template <typename... Args>
    void critical(std::string_view format, const Args &... args) {
      push(Level::CRITICAL, format, args...);
    }

    /**
     * Logs value {@param arg} as event with level of critical situation
     */
    template <typename Arg>
    void critical(const Arg &arg) {
      push(Level::CRITICAL, "{}", arg);
    }

    /**
     * Flushes all events accumulated in sink immediately
     */
    void flush() const {
      sink_->async_flush();
    }

    // Level

    /**
     * @returns current level of logging
     */
    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    /**
     * @returns true if level is overridden, and true if it is inherited from
     * group
     */
    [[nodiscard]] bool isLevelOverridden() const noexcept {
      return is_level_overridden_;
    }

    /**
     * Set level to level from group. Level will be marked as inherited
     */
    void resetLevel();

    /**
     * Set level to {@param level}. Level will be marked as overridden
     */
    void setLevel(Level level);

    /**
     * Set level to level from group {@param group}
     */
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * Set level to level from group with name {@param group_name}
     */
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    /**
     * @returns current sink of logging
     */
    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }

    /**
     * @returns true if sink is overridden, and true if it is inherited from
     * group
     */
    [[nodiscard]] bool isSinkOverridden() const noexcept {
      return is_sink_overridden_;
    }

    /**
     * Set sink to sink from group. Level will be marked as inherited
     */
    void resetSink();

    /**
     * Set sink to sink with name {@param sink_name}.
     * Level will be marked as overridden
     */
    void setSink(const std::string &sink_name);

    /**
     * Set sink to sink {@param sink}. Level will be marked as overridden
     */
    void setSink(std::shared_ptr<Sink> sink);

    /**
     * Set sink to sink from group {@param group}
     */
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * Set sink to sink from group with name {@param group_name}
     */
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    /**
     * @returns group of logger
     */
    [[nodiscard]] std::shared_ptr<const Group> group() const noexcept {
      return group_;
    }

    /**
     * Set group to group {@param group}.
     * Non overridden properties will me inherited from new group
     */
    void setGroup(std::shared_ptr<const Group> group);

    /**
     * Set group to group with name {@param group_name}.
     * Non overridden properties will me inherited from new group
     */
    void setGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;

    const std::string name_;
    std::shared_ptr<const Group> group_;

    std::shared_ptr<Sink> sink_;
    bool is_sink_overridden_{};

    Level level_{};
    bool is_level_overridden_{};
  };

}  // namespace soralog

#endif  // SORALOG_LOG
