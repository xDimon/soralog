/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_file.hpp>

#include <sysexits.h>
#include <chrono>
#include <iostream>

#include <fmt/chrono.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

    // Separator used between logical parts of a log record.
    // Can be any substring or character (space, tab, etc.).
    // A double space is chosen to distinguish it from a single space.
    constexpr std::string_view separator = "  ";

    /**
     * @brief Inserts a separator into the buffer.
     * @param ptr Reference to the pointer where the separator should be
     * written.
     */
    void put_separator(char *&ptr) {
      for (auto c : separator) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    /**
     * @brief Writes the textual representation of the log level into the
     * buffer.
     * @param ptr Reference to the pointer where the log level should be
     * written.
     * @param level The log level to be written.
     */
    void put_level(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      const char *const end = ptr + 8;  // Ensuring fixed width
      const char *str = levelToStr(level);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      while (auto c = *str++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
      while (ptr < end) {  // Pad with spaces to maintain alignment
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = ' ';
      }
    }

    /**
     * @brief Writes a single-character representation of the log level.
     * @param ptr Reference to the pointer where the character should be
     * written.
     * @param level The log level to be written.
     */
    void put_level_short(char *&ptr, Level level) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      *ptr++ = levelToChar(level);
    }

    /**
     * @brief Writes a string into the buffer.
     * @tparam T Type of the string (std::string, std::string_view, or
     * C-string).
     * @param ptr Reference to the pointer where the string should be written.
     * @param name The string to write.
     */
    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
      }
    }

    /**
     * @brief Writes a string into the buffer with a fixed width, padding if
     * necessary.
     * @tparam T Type of the string (std::string, std::string_view, or
     * C-string).
     * @param ptr Reference to the pointer where the string should be written.
     * @param name The string to write.
     * @param width The fixed width to write. If the string is shorter, it is
     * padded.
     */
    template <typename T>
    void put_string(char *&ptr, const T &name, size_t width) {
      if (width == 0) {
        return;
      }
      for (auto c : name) {
        // Stop at null terminator or when width is exhausted
        if (c == '\0' or width == 0) {
          break;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = c;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        -width;
      }
      while (width--) {  // Pad remaining space with spaces
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = ' ';
      }
    }

  }  // namespace

  SinkToFile::SinkToFile(std::string name,
                         Level level,
                         std::filesystem::path path,
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
             latency.value_or(1000),                 // Default: 1 sec
             at_fault.value_or(AtFaultReactionType::WAIT)),
        path_(std::move(path)),
        buff_(max_buffer_size_) {
    // Open the log file in append mode
    out_.open(path_, std::ios::app);
    if (not out_.is_open()) {
      // If the file cannot be opened, print an error message to stderr
      std::cerr << "Can't open log file '" << path_ << "': " << strerror(errno)
                << '\n';
    } else if (latency_ != std::chrono::milliseconds::zero()) {
      // If the sink operates with a delay (latency),
      // start the background worker thread
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToFile::~SinkToFile() {
    if (latency_ != std::chrono::milliseconds::zero()) {
      // If latency-based flushing is enabled,
      // signal the worker thread to finalize
      need_to_finalize_.store(true, std::memory_order_release);
      async_flush();
      if (sink_worker_ and sink_worker_->joinable()) {
        sink_worker_->join();
        sink_worker_.reset();
      }
    } else {
      // If there's no latency-based flushing,
      // perform an immediate flush on destruction
      flush();
    }
  }

  void SinkToFile::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      // If latency is enabled, set the flush flag and notify the worker thread
      need_to_flush_.store(true, std::memory_order_release);
      condvar_.notify_one();
    } else {
      // Otherwise, flush immediately
      flush();
    }
  }

  void SinkToFile::flush() noexcept {
    if (flush_in_progress_.test_and_set()) {
      return;  // Prevents concurrent flushes by checking and setting the flag
    }

    // Pointer to the begin of the buffer
    auto *const begin = buff_.data();
    // Pointer to the end of the buffer
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *const end = buff_.data() + buff_.size();
    auto *ptr = begin;

    // Stores the previous second to avoid redundant conversions
    decltype(1s / 1s) psec = 0;
    // Struct to store timestamp breakdown
    std::tm tm{};
    // Buffer for formatted timestamp "YY.MM.DD HH:MM:SS"
    std::array<char, 17> datetime{};

    while (true) {
      bool appended = false;
      if (auto node =
              events_.get()) {  // Retrieve an event from the circular buffer
        const auto &event = *node;

        // Extract timestamp details
        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        if (psec != sec) {  // Format timestamp only if second has changed
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

        // Write thread info
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

        // Write log level
        put_level(ptr, event.level());
        put_separator(ptr);

        // Write logger name
        put_string(ptr, event.name());
        put_separator(ptr);

        // Write log message
        put_string(ptr, event.message());
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *ptr++ = '\n';

        size_ -= event.message().size();  // Adjust buffer size
        appended = true;
      }

      // Check whether to flush buffer
      if ((end - ptr) < sizeof(Event) or appended
          or std::chrono::steady_clock::now()
                 >= next_flush_.load(std::memory_order_acquire)) {
        std::chrono::seconds attempt_delay{1};
        auto *data_begin = begin;
        auto *data_end = ptr;

        while (data_end > data_begin) {  // Retry writing to log file on failure
          next_flush_.store(std::chrono::steady_clock::now() + latency_,
                            std::memory_order_release);
          auto pos_before_write = out_.tellp();
          out_.write(data_begin, data_end - data_begin);

          if (out_.fail() or out_.bad()) {  // Handle I/O failures
            const auto *msg =
                out_.bad()
                    ? "Critical I/O error (disk full or hardware failure).\n"
                    : "Write failed (possibly out of disk space).\n";
            std::cerr << msg;
            std::cerr.flush();

            switch (at_fault_) {
              case AtFaultReactionType::TERMINATE:
                out_ << "Fatal: " << msg;
                out_.close();
                exit(EX_IOERR);

              case AtFaultReactionType::WAIT:
                std::this_thread::sleep_for(attempt_delay);
                attempt_delay =
                    std::min(attempt_delay * 2, std::chrono::seconds(60));
                if (out_.bad()) {
                  while (true) {  // Try reopening the log file
                    std::ofstream out;
                    out.open(path_, std::ios::app);
                    if (out.is_open()) {
                      std::swap(out_, out);
                      break;  // Leave loop of trying to reopen a log-file
                    }
                    std::cerr << "Can't " << (out_.is_open() ? "re-" : "")
                              << "open log file '" << path_
                              << "': " << strerror(errno) << ". Waiting for "
                              << attempt_delay.count() << " seconds...\n";
                    std::cerr.flush();
                  }
                } else {
                  out_.clear();
                  std::advance(data_begin, out_.tellp() - pos_before_write);
                }
                continue;

              case AtFaultReactionType::IGNORE:
                break;  // Ignore the failure
            }
          }
          break;  // Leave loop of trying to write an info log-file
        }
      }

      // Flush log file if necessary
      if (appended) {
        bool true_v = true;
        if (need_to_flush_.compare_exchange_weak(
                true_v, false, std::memory_order_acq_rel)) {
          out_.flush();
        }
      }

      break;
    }

    // Handle log file rotation if needed
    bool true_v = true;
    if (need_to_rotate_.compare_exchange_weak(
            true_v, false, std::memory_order_acq_rel)) {
      std::ofstream out;
      out.open(path_, std::ios::app);
      if (not out.is_open()) {
        if (out_.is_open()) {
          std::cerr << "Can't re-open log file '" << path_
                    << "': " << strerror(errno) << '\n';
        } else {
          std::cerr << "Can't open log file '" << path_
                    << "': " << strerror(errno) << '\n';
        }
        std::cerr.flush();
      } else {
        std::swap(out_, out);
      }
    }

    flush_in_progress_.clear();  // Reset the flush flag
  }

  void SinkToFile::rotate() noexcept {
    // Mark that a log rotation is needed
    need_to_rotate_.store(true, std::memory_order_release);
    // Ensure pending logs are flushed before rotating the file
    async_flush();
  }

  void SinkToFile::run() {
    // Set the thread name for easier debugging
    util::setThreadName("log:" + name_);

    // Initialize the next flush time
    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);
        // Wait until the next flush time or until a condition is notified
        if (condvar_.wait_until(lock,
                                next_flush_.load(std::memory_order_relaxed))
            == std::cv_status::no_timeout) {
          // If neither flushing nor finalization is needed, continue waiting
          if (not need_to_flush_.load(std::memory_order_relaxed)
              and not need_to_finalize_.load(std::memory_order_relaxed)) {
            continue;
          }
        }
      }

      // Perform a flush of buffered log events
      flush();

      // If finalization is requested and the event queue is empty,
      // exit the loop
      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }

}  // namespace soralog
