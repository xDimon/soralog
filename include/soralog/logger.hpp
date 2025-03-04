/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>

#include <soralog/level.hpp>
#include <soralog/sink.hpp>

namespace soralog {

  class LoggingSystem;
  class Group;

  /**
   * @class Logger
   * @brief Filters events by level and sends them to a logging sink.
   */
  class Logger final {
   public:
    Logger() = delete;
    Logger(Logger &&) noexcept = delete;
    Logger(const Logger &) = delete;
    ~Logger() = default;
    Logger &operator=(Logger &&) noexcept = delete;
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Constructs a logger instance.
     * @param system Reference to the logging system.
     * @param logger_name Logger name.
     * @param group Logger group.
     */
    Logger(LoggingSystem &system,
           std::string logger_name,
           std::shared_ptr<const Group> group);

   private:
    /**
     * @brief Logs an event if its level is sufficient.
     * @tparam Format Format string type.
     * @tparam Args Additional argument types.
     * @param level Log level.
     * @param format Format string.
     * @param args Formatting arguments.
     */
    template <typename Format, typename... Args>
    void __attribute__((no_sanitize("thread"))) push(Level level,
                                                     const Format &format,
                                                     const Args &...args) {
      if (level_ >= level) {
        if (level != Level::OFF and level != Level::IGNORE) {
          sink_->push(name_, level, format, args...);
          if (level_ >= Level::CRITICAL) {
            sink_->flush();
          }
        }
      }
    }

   public:
    /**
     * @brief Gets the logger's name.
     * @return Logger name.
     */
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    /**
     * @brief Logs an event at the specified level.
     * @tparam Format Format string type.
     * @tparam Args Additional argument types.
     * @param level Log level.
     * @param format Format string.
     * @param args Formatting arguments.
     */
    template <typename Format, typename... Args>
    void log(Level level, Format &&format, const Args &...args) {
      push(level, std::forward<Format>(format), args...);
    }

    /// Logs an event with TRACE level.
    template <typename... Args>
    void trace(std::string_view format, const Args &...args) {
      push(Level::TRACE, format, args...);
    }

    /// Logs a single value with TRACE level.
    template <typename Arg>
    void trace(const Arg &arg) {
      push(Level::TRACE, "{}", arg);
    }

    /// Logs an event with DEBUG level.
    template <typename... Args>
    void debug(std::string_view format, const Args &...args) {
      push(Level::DEBUG, format, args...);
    }

    /// Logs a single value with DEBUG level.
    template <typename Arg>
    void debug(const Arg &arg) {
      push(Level::DEBUG, "{}", arg);
    }

    /// Logs an event with VERBOSE level.
    template <typename... Args>
    void verbose(std::string_view format, const Args &...args) {
      push(Level::VERBOSE, format, args...);
    }

    /// Logs a single value with VERBOSE level.
    template <typename Arg>
    void verbose(const Arg &arg) {
      push(Level::VERBOSE, "{}", arg);
    }

    /// Logs an event with INFO level.
    template <typename... Args>
    void info(std::string_view format, const Args &...args) {
      push(Level::INFO, format, args...);
    }

    /// Logs a single value with INFO level.
    template <typename Arg>
    void info(const Arg &arg) {
      push(Level::INFO, "{}", arg);
    }

    /// Logs an event with WARN level.
    template <typename... Args>
    void warn(std::string_view format, const Args &...args) {
      push(Level::WARN, format, args...);
    }

    /// Logs a single value with WARN level.
    template <typename Arg>
    void warn(const Arg &arg) {
      push(Level::WARN, "{}", arg);
    }

    /// Logs an event with ERROR level.
    template <typename... Args>
    void error(std::string_view format, const Args &...args) {
      push(Level::ERROR, format, args...);
    }

    /// Logs a single value with ERROR level.
    template <typename Arg>
    void error(const Arg &arg) {
      push(Level::ERROR, "{}", arg);
    }

    /// Logs an event with CRITICAL level.
    template <typename... Args>
    void critical(std::string_view format, const Args &...args) {
      push(Level::CRITICAL, format, args...);
    }

    /// Logs a single value with CRITICAL level.
    template <typename Arg>
    void critical(const Arg &arg) {
      push(Level::CRITICAL, "{}", arg);
    }

    /**
     * @brief Flushes all pending log events.
     */
    void flush() const {
      sink_->flush();
    }

    // Level

    /// Gets the current logging level.
    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    /// Checks if the logging level is overridden.
    [[nodiscard]] bool isLevelOverridden() const noexcept {
      return is_level_overridden_;
    }

    /// Resets the logging level to inherit from the group.
    void resetLevel();

    /// Sets the logging level and marks it as overridden.
    void setLevel(Level level);

    /// Sets the logging level from another group.
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);

    /// Sets the logging level from a group by name.
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    /// Gets the current logging sink.
    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }

    /// Checks if the sink is overridden.
    [[nodiscard]] bool isSinkOverridden() const noexcept {
      return is_sink_overridden_;
    }

    /// Resets the sink to inherit from the group.
    void resetSink();

    /// Sets the sink by name and marks it as overridden.
    void setSink(const std::string &sink_name);

    /// Sets the sink and marks it as overridden.
    void setSink(std::shared_ptr<Sink> sink);

    /// Sets the sink from another group.
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);

    /// Sets the sink from a group by name.
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    /// Gets the logger's group.
    [[nodiscard]] std::shared_ptr<const Group> group() const noexcept {
      return group_;
    }

    /// Sets the logger's group.
    void setGroup(std::shared_ptr<const Group> group);

    /// Sets the logger's group by name.
    void setGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;  ///< Reference to the logging system.

    const std::string name_;              ///< Logger name.
    std::shared_ptr<const Group> group_;  ///< Logger group.

    std::shared_ptr<Sink> sink_;  ///< Logger sink.
    bool is_sink_overridden_{};   ///< Is sink overridden.

    Level level_{};               ///< Logging level.
    bool is_level_overridden_{};  ///< Is level overridden.
  };

}  // namespace soralog
