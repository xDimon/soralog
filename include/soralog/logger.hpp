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
   * @brief Filters events by level and forwards them to a Sink.
   *
   * @details
   * Logger is a lightweight front-end that:
   * - checks whether a message should be logged according to the current
   *   effective level (see level());
   * - forwards the message to the configured Sink (see sink()).
   *
   * The logger may inherit its configuration (level and sink) from a Group,
   * unless those values were overridden explicitly for this logger.
   *
   * @par Special level semantics
   * - Level::OFF and Level::IGNORE are treated as disabled levels and are not
   *   forwarded to the sink.
   * - Level::CRITICAL triggers a flush of the current sink after the message
   *   is pushed.
   * - Level::FATAL triggers flushing of all sinks registered in the owning
   *   LoggingSystem and then terminates the process (see doTerminate()).
   *
   * @warning
   * Logging at Level::FATAL terminates the process unconditionally.
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
     *
     * @param system Owning logging system.
     * @param logger_name Human-readable logger name (typically a component
     * name).
     * @param group Parent group used for default configuration inheritance.
     */
    Logger(LoggingSystem &system,
           std::string logger_name,
           std::shared_ptr<const Group> group);

   private:
    /**
     * @brief Flushes all sinks and terminates the process.
     *
     * @details
     * This is invoked after a message with Level::FATAL has been pushed to the
     * current sink. The implementation flushes all sinks registered in the
     * owning LoggingSystem and then calls std::abort().
     *
     * This function never returns.
     */
    [[noreturn]] void doTerminate() const;

    /**
     * @brief Internal logging fast path.
     *
     * @details
     * This function implements the common logic for all logging entry points:
     * - checks that the current effective logger level allows the message;
     * - filters out OFF/IGNORE levels;
     * - forwards the message to the sink;
     * - performs post-actions for CRITICAL and FATAL levels:
     *   - CRITICAL: flushes the current sink;
     *   - FATAL: flushes all sinks via LoggingSystem and aborts the process.
     *
     * @note
     * ThreadSanitizer is disabled for this function to avoid false positives
     * on the logging fast path. Thread-safety properties are defined by the
     * Sink and the LoggingSystem implementation.
     *
     * @tparam Format Format/message type. The logger does not impose
     * restrictions on it. Typical inputs are string literals, std::string,
     *                std::string_view, and C-strings. The sink is responsible
     *                for interpreting the provided format and arguments.
     * @tparam Args Formatting argument types.
     *
     * @param level Requested log level for this message.
     * @param format Format string or message.
     * @param args Formatting arguments.
     */
    template <typename Format, typename... Args>
    void __attribute__((no_sanitize("thread"))) push(Level level,
                                                     const Format &format,
                                                     const Args &...args) {
      // Check whether this event is allowed by the current logger level
      if (level_ >= level) {
        // Ignore disabled levels explicitly
        if (level_ != Level::OFF and level != Level::IGNORE) [[likely]] {
          // Forward the event to the sink
          sink_->push(name_, level, format, args...);

          // CRITICAL: flush the current sink
          if (level == Level::CRITICAL) [[unlikely]] {
            sink_->flush();
          }

          // FATAL: flush all sinks and terminate the process
          if (level == Level::FATAL) [[unlikely]] {
            doTerminate();
          }
        }
      }
    }

   public:
    /**
     * @brief Returns the logger name.
     *
     * @details
     * The name is typically used to identify the component that produced the
     * message and is passed to the sink with each event.
     */
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    /**
     * @brief Generic logging entry point (used by macros).
     *
     * @details
     * Forwards the message at the specified level to the internal fast path.
     * This overload intentionally accepts a wide range of format/message types.
     *
     * @tparam Format Format/message type.
     * @tparam Args Formatting argument types.
     *
     * @param level Requested log level.
     * @param format Format string or message.
     * @param args Formatting arguments.
     */
    template <typename Format, typename... Args>
    void log(Level level, Format &&format, const Args &...args) {
      push(level, std::forward<Format>(format), args...);
    }

    /**
     * @name Convenience helpers by level
     * @details
     * These helpers accept std::string_view and forward to push().
     * For logging a single value, the overloads below format it with "{}".
     */
    ///@{

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

    /**
     * @brief Logs an event with CRITICAL level and flushes the current sink.
     *
     * @details
     * Unlike other levels, CRITICAL forces Sink::flush() after the event is
     * pushed to minimize the chance of losing the last messages on abnormal
     * shutdown.
     */
    template <typename... Args>
    void critical(std::string_view format, const Args &...args) {
      push(Level::CRITICAL, format, args...);
    }

    /// Logs a single value with CRITICAL level and flushes the sink.
    template <typename Arg>
    void critical(const Arg &arg) {
      push(Level::CRITICAL, "{}", arg);
    }

    /**
     * @brief Logs an event with FATAL level and terminates the process.
     *
     * @details
     * The event is first pushed to the current sink, then all sinks are flushed
     * via the owning LoggingSystem, and finally the process is aborted.
     *
     * @warning This function does not return.
     */
    template <typename... Args>
    void fatal(std::string_view format, const Args &...args) {
      push(Level::FATAL, format, args...);
    }

    /// Logs a single value with FATAL level using "{}" and terminates the
    /// process.
    template <typename Arg>
    void fatal(const Arg &arg) {
      push(Level::FATAL, "{}", arg);
    }

    ///@}

    /**
     * @brief Flushes the current sink.
     *
     * @details
     * Forces the underlying sink to flush any buffered output.
     * This does not change the configured level or sink.
     */
    void flush() const {
      sink_->flush();
    }

    // Level

    /**
     * @brief Returns the current effective logging level.
     *
     * @details
     * This is the level used by the logger to decide whether a message should
     * be forwarded to the sink (see push()).
     */
    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    /// Returns true if the level was explicitly set for this logger.
    [[nodiscard]] bool isLevelOverridden() const noexcept {
      return is_level_overridden_;
    }

    /// Resets the level to inherit from the current group.
    void resetLevel();

    /// Sets the logger level and marks it as overridden.
    void setLevel(Level level);

    /// Sets the logger level from another group.
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);

    /// Sets the logger level from a group by name.
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    /// Returns the current sink used by this logger.
    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }

    /// Returns true if the sink was explicitly set for this logger.
    [[nodiscard]] bool isSinkOverridden() const noexcept {
      return is_sink_overridden_;
    }

    /// Resets the sink to inherit from the current group.
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

    /**
     * @brief Returns the current group used for configuration inheritance.
     */
    [[nodiscard]] std::shared_ptr<const Group> group() const noexcept {
      return group_;
    }

    /// Sets the logger group.
    void setGroup(std::shared_ptr<const Group> group);

    /// Sets the logger group by name.
    void setGroup(const std::string &group_name);

   private:
    /// Owning logging system.
    LoggingSystem &system_;

    /// Logger name (component identifier).
    const std::string name_;
    /// Group for configuration inheritance.
    std::shared_ptr<const Group> group_;

    /// Current sink used for output.
    std::shared_ptr<Sink> sink_;
    /// True if sink was explicitly overridden.
    bool is_sink_overridden_{};

    /// Current effective logger level.
    Level level_{};
    /// True if level was explicitly overridden.
    bool is_level_overridden_{};
  };

}  // namespace soralog
