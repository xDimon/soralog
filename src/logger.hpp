/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_LOG
#define XLOG_LOG

#include <memory>
#include <string>
#include <string_view>

#include <boost/assert.hpp>

#include <event.hpp>
#include <sink.hpp>

namespace xlog {

  class Logger final {
   public:
    Logger() = delete;
    Logger(Logger &&) noexcept = delete;
    Logger(const Logger &) = delete;
    ~Logger() = default;
    Logger &operator=(Logger &&) noexcept = delete;
    Logger &operator=(Logger const &) = delete;

    Logger(std::string name, std::shared_ptr<Sink> sink, Level level)
        : name_(std::move(name)), sink_(std::move(sink)), level_(level) {
      BOOST_ASSERT(sink_);
    }

   private:
    template <typename... Args>
    void push(Level level, std::string_view format, Args... args) {
      if (level_ >= level) {
        sink_->push(name_, level, format, std::forward<Args>(args)...);
      }
    }

   public:
    [[nodiscard]] const std::string &name() const {
      return name_;
    }

    [[nodiscard]] Level level() const {
      return level_;
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

   private:
    std::string name_;
    std::shared_ptr<Sink> sink_;
    Level level_;
  };

  using Log = std::shared_ptr<Logger>;

}  // namespace xlog

#endif  // XLOG_LOG
