/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_console.hpp>

#include <chrono>
#include <iostream>
#include <string_view>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

    // Separator used between logical parts of a log record.
    // A double space is chosen to distinguish from a single space.
    constexpr std::string_view separator = "  ";

    // Escape sequence to reset terminal text formatting to default.
    constexpr std::string_view reset_color = "\x1b[0m";

    // Mapping log levels to corresponding terminal colors.
    constexpr std::array<fmt::color, static_cast<size_t>(Level::TRACE) + 1>
        level_to_color_map{
            fmt::color::brown,         // OFF
            fmt::color::red,           // CRITICAL
            fmt::color::orange_red,    // ERROR
            fmt::color::orange,        // WARNING
            fmt::color::forest_green,  // INFO
            fmt::color::dark_green,    // VERBOSE
            fmt::color::medium_blue,   // DEBUG
            fmt::color::gray,          // TRACE
        };

    // Helper function to process multiple arguments.
    template <typename... Args>
    inline void pass(const Args...) {}

    // Dummy enum to use in template specialization.
    enum V {};

    /**
     * @brief Applies a text style to the log message.
     *
     * Copies the style bytes into the character buffer and advances the
     * pointer.
     */
    template <typename T>
    V put_style(char *&ptr, T style) {
      auto size = std::end(style) - std::begin(style);
      std::memcpy(ptr, std::begin(style), size);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ptr += size;
      return {};
    }

    /**
     * @brief Applies multiple text styles in sequence.
     */
    template <typename... Args>
    void put_style(char *&ptr, Args &&...styles) {
      pass(put_style(ptr, std::forward<Args>(styles))...);
    };

    /**
     * @brief Resets the text style to default using an ANSI escape sequence.
     */
    void put_reset_style(char *&ptr) {
      const auto &style = reset_color;
      auto size = std::end(style) - std::begin(style);
      std::memcpy(ptr, std::begin(style), size);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ptr = ptr + size;
    }

    /**
     * @brief Sets the text style for a log level.
     *
     * Critical and error levels are highlighted in bold colors.
     */
    void put_level_style(char *&ptr, Level level) {
      assert(level <= Level::TRACE);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      auto color = level_to_color_map[static_cast<size_t>(level)];
      put_style(ptr,
                fmt::detail::make_foreground_color<char>(color),
                fmt::detail::make_emphasis<char>(fmt::emphasis::bold));
    }

    /**
     * @brief Applies formatting to the logger name.
     *
     * Logger names are displayed in bold for better visibility.
     */
    void put_name_style(char *&ptr) {
      put_style(ptr, fmt::detail::make_emphasis<char>(fmt::emphasis::bold));
    }

    /**
     * @brief Applies different text styles depending on the log level.
     *
     * - Critical and error messages are bold.
     * - Debug and trace messages are italicized.
     */
    void put_text_style(char *&ptr, Level level) {
      assert(level <= Level::TRACE);
      if (level <= Level::ERROR) {
        put_style(ptr, fmt::detail::make_emphasis<char>(fmt::emphasis::bold));
      } else if (level >= Level::DEBUG) {
        put_style(ptr, fmt::detail::make_emphasis<char>(fmt::emphasis::italic));
      }
    }

    /**
     * @brief Inserts a separator between log record fields.
     */
    void put_separator(char *&ptr) {
      for (auto c : separator) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    /**
     * @brief Writes the full string representation of a log level.
     *
     * Ensures that the level is padded to a fixed width of 8 characters.
     */
    void put_level(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      const char *const end = ptr + 8;
      const char *str = levelToStr(level);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      while (auto c = *str++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;  // Copy characters one by one.
      }
      while (ptr < end) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = ' ';  // Pad with spaces to maintain alignment.
      }
    }

    /**
     * @brief Writes a single-character representation of a log level.
     */
    void put_level_short(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      *ptr++ = levelToChar(level);
    }

    /**
     * @brief Copies a string into the buffer without width constraints.
     */
    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    /**
     * @brief Copies a string into the buffer, ensuring a fixed width.
     *
     * If the string is shorter than the given width, it is padded with spaces.
     */
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
        *ptr++ = ' ';  // NOLINT  Pad remaining space with spaces.
      }
    }

  }  // namespace

  SinkToConsole::SinkToConsole(std::string name,
                               Level level,
                               Stream stream_type,
                               bool with_color,
                               std::optional<ThreadInfoType> thread_info_type,
                               std::optional<size_t> capacity,
                               std::optional<size_t> max_message_length,
                               std::optional<size_t> buffer_size,
                               std::optional<size_t> latency,
                               std::optional<AtFaultReactionType> at_fault)
      : Sink(std::move(name),
             level,
             thread_info_type.value_or(ThreadInfoType::NONE),
             capacity.value_or(1u << 6),  // Default: 64 events in queue
             max_message_length.value_or(
                 1u << 10),                   // Default: 1024 bytes per message
             buffer_size.value_or(1u << 17),  // Default: 128 KB buffer
             latency.value_or(200),           // Default: 200 ms latency
             at_fault.value_or(AtFaultReactionType::IGNORE)),
        stream_(stream_type == Stream::STDERR
                    ? std::cerr
                    : std::cout),  // Selects output stream
        with_color_(with_color),   // Enables or disables colored output
        buff_(max_buffer_size_) {  // Allocates buffer memory
    // If a nonzero latency is set, start a worker thread for async logging.
    if (latency_ != std::chrono::milliseconds::zero()) {
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToConsole::~SinkToConsole() {
    // If async logging is enabled, notify the worker thread to finalize.
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_finalize_.store(true, std::memory_order_release);
      async_flush();
      if (sink_worker_ and sink_worker_->joinable()) {
        sink_worker_->join();
        sink_worker_.reset();
      }
    } else {
      // If async logging is disabled, flush logs before destruction.
      flush();
    }
  }

  void SinkToConsole::async_flush() noexcept {
    // If latency is nonzero, request the worker thread to flush logs.
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_flush_.store(true, std::memory_order_release);
      condvar_.notify_one();  // Wake up the worker thread for flushing.
    } else {
      // Otherwise, perform immediate flushing in the main thread.
      flush();
    }
  }

  void SinkToConsole::flush() noexcept {
    // Ensure that only one thread performs flushing at a time.
    if (flush_in_progress_.test_and_set()) {
      return;
    }

    auto *const begin = buff_.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *const end = buff_.data() + buff_.size();
    auto *ptr = begin;

    decltype(1s / 1s) psec = 0;  // Stores previous second timestamp.
    std::tm tm{};

    // Formatted timestamp: "00.00.00 00:00:00"
    std::array<char, 17> datetime{};

    while (true) {
      bool appended = false;
      if (auto node = events_.get()) {  // Retrieve the next log event.
        const auto &event = *node;

        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        // Format timestamp only when the second changes.
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

        // Append timestamp to buffer.
        std::memcpy(ptr, datetime.data(), datetime.size());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ptr = ptr + datetime.size();

        // Apply color if enabled.
        if (with_color_) {
          const auto &style =
              fmt::detail::make_foreground_color<char>(fmt::color::gray);
          auto size = std::end(style) - std::begin(style);
          std::memcpy(ptr, std::begin(style), size);
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          ptr = ptr + size;
        }

        // Append microseconds.
        ptr = fmt::format_to_n(ptr, end - ptr, ".{:0>6}", usec).out;

        if (with_color_) {
          put_reset_style(ptr);  // Reset text style.
        }

        put_separator(ptr);

        // Append thread info based on configuration.
        switch (thread_info_type_) {
          case ThreadInfoType::NAME:
            put_string(ptr, event.thread_name(), 15);
            put_separator(ptr);
            break;
          case ThreadInfoType::ID:
            ptr = fmt::format_to_n(
                      ptr, end - ptr, "T:{:<6}", event.thread_number())
                      .out;
            put_separator(ptr);
            break;
          default:
            break;
        }

        // Append log level.
        if (with_color_) {
          put_level_style(ptr, event.level());
        }
        put_level(ptr, event.level());
        if (with_color_) {
          put_reset_style(ptr);
        }

        put_separator(ptr);

        // Append logger name.
        if (with_color_) {
          put_name_style(ptr);
        }
        put_string(ptr, event.name());
        if (with_color_) {
          put_reset_style(ptr);
        }

        put_separator(ptr);

        // Append log message.
        if (with_color_) {
          put_text_style(ptr, event.level());
        }
        put_string(ptr, event.message());
        if (with_color_) {
          put_reset_style(ptr);
        }

        *ptr++ = '\n';  // NOLINT - Add a new line after the log entry.

        size_ -= event.message().size();  // Reduce buffer size usage.
        appended = true;
      }

      // Flush buffer if it's full, if a message was added,
      // or if it's time to flush.
      if ((end - ptr) < sizeof(Event) or appended
          or std::chrono::steady_clock::now()
                 >= next_flush_.load(std::memory_order_acquire)) {
        next_flush_.store(std::chrono::steady_clock::now() + latency_,
                          std::memory_order_release);
        stream_.write(begin, ptr - begin);  // Output to console.
        ptr = begin;                        // Reset buffer pointer.
      }

      // If at least one message was processed, check if flushing is needed.
      if (appended) {
        bool true_v = true;
        if (need_to_flush_.compare_exchange_weak(
                true_v, false, std::memory_order_acq_rel)) {
          stream_.flush();  // Ensure logs are printed immediately.
        }
      }

      break;  // Exit loop since all pending messages were processed.
    }

    flush_in_progress_.clear();  // Allow other threads to flush.
  }

  void SinkToConsole::run() {
    // Set the thread name for debugging and logging purposes.
    util::setThreadName("log:" + name_);

    // Initialize the next flush time with the current time.
    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);
        // Wait until the next scheduled flush time or until a flush request is
        // received.
        if (condvar_.wait_until(lock,
                                next_flush_.load(std::memory_order_relaxed))
            == std::cv_status::no_timeout) {
          // If no flush is requested and no finalization is needed, continue
          // waiting.
          if (not need_to_flush_.load(std::memory_order_relaxed)
              and not need_to_finalize_.load(std::memory_order_relaxed)) {
            continue;
          }
        }
      }

      // Perform flushing of accumulated log messages.
      flush();

      // If finalization is requested and all events are processed, exit the
      // loop.
      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }

}  // namespace soralog
