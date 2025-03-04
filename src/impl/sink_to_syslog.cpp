/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_syslog.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>

#include <fmt/chrono.h>
#include <syslog.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

    // Separator used between logical parts of a log record.
    // It can be any substring or symbol: space, tab, etc.
    // Double space is chosen to differentiate from a single space.
    constexpr std::string_view separator = "  ";

    // Appends the separator sequence to the buffer
    void put_separator(char *&ptr) {
      for (auto c : separator) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    // Writes the log level string representation to the buffer
    void put_level(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      const char *const end = ptr + 8;
      const char *str = levelToStr(level);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      while (auto c = *str++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    // Writes the short log level character representation to the buffer
    void put_level_short(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      *ptr++ = levelToChar(level);
    }

    // Writes a string to the buffer
    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    // Writes a string to the buffer with a specified width, padding if needed
    template <typename T>
    void put_string(char *&ptr, const T &name, size_t width) {
      if (width == 0) {
        return;
      }
      for (auto c : name) {
        if (c == '\0' or width == 0) {
          break;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
        --width;
      }
      while (width--) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = ' ';
      }
    }

  }  // namespace

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  std::atomic_bool SinkToSyslog::syslog_is_opened_{false};

  SinkToSyslog::SinkToSyslog(std::string name,
                             Level level,
                             std::string ident,
                             std::optional<ThreadInfoType> thread_info_type,
                             std::optional<size_t> capacity,
                             std::optional<size_t> max_message_length,
                             std::optional<size_t> buffer_size,
                             std::optional<size_t> latency,
                             std::optional<AtFaultReactionType> at_fault)
      : Sink(std::move(name),
             level,
             thread_info_type.value_or(ThreadInfoType::NONE),
             capacity.value_or(1u << 11),            // Default: 2048 events
             max_message_length.value_or(1u << 10),  // Default: 1024 bytes
             buffer_size.value_or(1u << 22),         // Default: 4 MB
             latency.value_or(1000),                 // Default: 1 second
             at_fault.value_or(AtFaultReactionType::IGNORE)),
        ident_(std::move(ident)),
        buff_(max_buffer_size_) {
    bool false_v = false;
    // Ensures only one instance of SinkToSyslog can be active at a time
    if (not syslog_is_opened_.compare_exchange_strong(
            false_v, true, std::memory_order_acq_rel)) {
      throw std::runtime_error(
          "SinkToSyslog has not been created: Syslog is already open");
    }

    // Initialize syslog with process ID logging enabled
    openlog(ident_.c_str(), LOG_PID | LOG_NDELAY, LOG_USER);

    // If latency is configured, start the worker thread for asynchronous
    // logging
    if (latency_ != std::chrono::milliseconds::zero()) {
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToSyslog::~SinkToSyslog() {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_finalize_.store(true, std::memory_order_release);
      async_flush();
      if (sink_worker_ and sink_worker_->joinable()) {
        sink_worker_->join();
        sink_worker_.reset();
      }
    } else {
      flush();
    }
    closelog();
    syslog_is_opened_.store(true, std::memory_order_release);
  }

  void SinkToSyslog::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_flush_.store(true, std::memory_order_release);
      condvar_.notify_one();
    } else {
      flush();
    }
  }

  void SinkToSyslog::flush() noexcept {
    if (flush_in_progress_.test_and_set()) {
      return;
    }

    auto *const begin = buff_.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *const end = buff_.data() + buff_.size();
    auto *ptr = begin;

    decltype(1s / 1s) psec = 0;
    std::tm tm{};
    std::array<char, 17> datetime{};  // "00.00.00 00:00:00"

    while (true) {
      auto node = events_.get();
      if (node) {
        const auto &event = *node;

        // Extract timestamp
        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        // Convert timestamp to a formatted datetime string
        if (psec != sec) {
          tm = fmt::localtime(sec);
          fmt::format_to_n(datetime.data(),
                           datetime.size(),
                           "{:0>2}.{:0>2}.{:0>2} {:0>2}:{:0>2}:{:0>2}",
                           tm.tm_year % 100,
                           tm.tm_mon + 1,
                           tm.tm_mday,
                           tm.tm_hour,
                           tm.tm_min,
                           tm.tm_sec);
          psec = sec;
        }

        // Write timestamp
        std::memcpy(ptr, datetime.data(), datetime.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ptr = ptr + datetime.size();
        ptr = fmt::format_to_n(ptr, end - ptr, ".{:0>6}", usec).out;
        put_separator(ptr);

        // Write thread information
        switch (thread_info_type_) {
          case ThreadInfoType::NAME:
            put_string(ptr, event.thread_name());
            put_separator(ptr);
            break;

          case ThreadInfoType::ID:
            ptr =
                fmt::format_to_n(ptr, end - ptr, "T:{}", event.thread_number())
                    .out;
            put_separator(ptr);
            break;

          default:
            break;
        }

        // Write log level
        put_level(ptr, event.level());
        put_separator(ptr);

        // Write logger name
        put_string(ptr, event.name());
        put_separator(ptr);

        // Write log message
        put_string(ptr, event.message());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = '\0';

        // Determine syslog priority based on log level
        bool must_log = true;
        int priority = 8;
        switch (event.level()) {
          case Level::OFF:
            must_log = false;
            break;
          case Level::CRITICAL:
            priority = LOG_EMERG;  // System is unusable
            break;
          case Level::ERROR:
            priority = LOG_ALERT;  // Error conditions
            break;
          case Level::WARN:
            priority = LOG_WARNING;  // Warning conditions
            break;
          case Level::INFO:
            priority = LOG_NOTICE;  // Normal but significant condition
            break;
          case Level::VERBOSE:
            priority = LOG_INFO;  // Informational
            break;
          case Level::DEBUG:
            priority = LOG_DEBUG;  // Debug-level messages
            break;
          case Level::TRACE:  // syslog must not log trace messages
            [[fallthrough]];
          default:
            must_log = false;
            break;
        }

        if (must_log) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
          syslog(priority, "%s", begin);
        }

        size_ -= event.message().size();
      }

      if (not node) {
        need_to_flush_.store(false, std::memory_order_release);
        break;
      }
    }

    flush_in_progress_.clear();
  }

  void SinkToSyslog::run() {
    // Set the thread name for debugging purposes
    util::setThreadName("log:" + name_);

    // Initialize the next flush time to the current moment
    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);

        // Wait until the next scheduled flush time or until explicitly notified
        if (condvar_.wait_until(lock,
                                next_flush_.load(std::memory_order_relaxed))
            == std::cv_status::no_timeout) {
          // If no flush or finalization is needed, continue waiting
          if (not need_to_flush_.load(std::memory_order_relaxed)
              and not need_to_finalize_.load(std::memory_order_relaxed)) {
            continue;
          }
        }
      }

      // Perform a flush operation to write logs to syslog
      flush();

      // If finalization is requested and there are
      // no pending events, exit the loop
      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }
}  // namespace soralog
