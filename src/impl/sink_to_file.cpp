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
#include <thread>

#include <soralog/internals.hpp>

namespace soralog {

  namespace {

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
        --width;
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
      {
        // Under the lock: the worker checks the flags before it falls asleep
        std::scoped_lock lock(mutex_);
        need_to_finalize_.store(true, std::memory_order_release);
      }
      condvar_.notify_one();
      if (sink_worker_ and sink_worker_->joinable()) {
        sink_worker_->join();
        sink_worker_.reset();
      }
    } else {
      // If there's no latency-based flushing,
      // perform an immediate flush on destruction
      sync_flush();
    }
  }

  void SinkToFile::flush() noexcept {
    sync_flush();
  }

  void SinkToFile::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      // If latency is enabled, set the flush flag and notify the worker thread
      // Wake the worker only when the request appears: the flag is reset when
      // a flush takes the request, until then repeated requests are no-ops
      if (not need_to_flush_.load(std::memory_order_relaxed)
          and not need_to_flush_.exchange(true, std::memory_order_acq_rel)) {
        // Under the lock: the worker checks the flags before it falls asleep
        std::scoped_lock lock(mutex_);
        condvar_.notify_one();
      }
    } else {
      // Otherwise, flush in the calling thread.
      internal_flush();
    }
  }

  void SinkToFile::sync_flush() noexcept {
    need_to_flush_.store(true, std::memory_order_release);
    internal_flush(true);
  }

  void SinkToFile::internal_flush(bool wait, bool bounded) noexcept {
    // Runs a flush; called with flush_in_progress_ set
    auto flush_once = [this, bounded] {
      const auto number =
          flushes_started_.fetch_add(1, std::memory_order_relaxed) + 1;
      // Pairs with the fence in the waiting branch: either the waiter sees
      // this flush started, or this flush sees the events of the waiter
      std::atomic_thread_fence(std::memory_order_seq_cst);
      // Take the request to flush the stream before counting the events: the
      // requester has queued its events before, so they are counted
      const bool flush_stream =
          need_to_flush_.exchange(false, std::memory_order_acq_rel);
      const auto result = flush_events(
          bounded ? events_.size() : events_.capacity(), flush_stream);
      flushes_done_.store(number, std::memory_order_release);
      return result;
    };
    bool reopened = false;
    if (wait) {
      // The caller wants its events written out on return. A flush takes the
      // events queued before it started, so the first flush started after
      // this point does it, whoever runs it: wait for one, or run it here.
      // Pairs with the fence in flush_once()
      std::atomic_thread_fence(std::memory_order_seq_cst);
      const auto started = flushes_started_.load(std::memory_order_relaxed);
      for (size_t attempt = 0;
           flush_in_progress_.test_and_set(std::memory_order_acquire);
           ++attempt) {
        if (flushes_done_.load(std::memory_order_acquire) > started) {
          return;  // Done by another thread
        }
        if (attempt >= 64) {
          std::this_thread::yield();
        }
      }
      reopened = flush_once();
      flush_in_progress_.clear(std::memory_order_release);
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
      do {
        // Pairs with the fence below: either this call sees the flag cleared,
        // or the call that clears it sees the events published before this one
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if (flush_in_progress_.test_and_set(std::memory_order_acquire)) {
          break;  // Another thread is flushing
        }
        reopened |= flush_once();
        flush_in_progress_.clear(std::memory_order_release);
        // An event published while the flag was held may have been missed
        // both by flush_events() and by its own producer, which found the flag
        // set. The worker takes it; a bounded flush leaves it to the producer,
        // which flushes synchronously after queueing anyway
        std::atomic_thread_fence(std::memory_order_seq_cst);
      } while (not bounded and events_.has_ready());
    }

    // Outside of the flush: push() may flush synchronously and wait for it
    if (reopened) {
      push("Soralog", Level::INFO, "Log-file has been reopened");
    }
  }

  bool SinkToFile::flush_events(size_t limit, bool flush_stream) noexcept {
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
      if (auto node = take_event(
              limit)) {  // Retrieve an event from the circular buffer
        const auto &event = *node;

        // Extract timestamp details
        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        if (psec != sec) {  // Format timestamp only if second has changed
          tm = detail::localtime(sec);
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

        appended = true;
      }

      // Write the buffer out when the next event might not fit into it
      // (sizeof(Event) covers the line prefix, the message is added on top),
      // when the queue is drained, or when the latency deadline has passed
      if (static_cast<size_t>(end - ptr) < sizeof(Event) + max_message_length_
          or not appended
          or std::chrono::steady_clock::now()
                 >= next_flush_.load(std::memory_order_acquire)) {
        std::chrono::seconds attempt_delay{1};
        auto *data_begin = begin;
        auto *data_end = ptr;

        // Move the deadline even if there is nothing to write, otherwise
        // the idle worker would wake up immediately again and again
        next_flush_.store(std::chrono::steady_clock::now() + latency_,
                          std::memory_order_release);

        while (data_end > data_begin) {  // Retry writing to log file on failure
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
          break;  // Leave loop of trying to write into log-file
        }
        ptr = begin;  // Buffer is written out, start filling it anew
      }

      if (appended) {
        continue;  // Take the next event
      }

      // Queue is drained: push written data to the file if requested
      if (flush_stream) {
        out_.flush();
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
        return false;
      }
      std::swap(out_, out);
      return true;
    }
    return false;
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
        condvar_.wait_until(
            lock, next_flush_.load(std::memory_order_relaxed), [this] {
              return need_to_flush_.load(std::memory_order_relaxed)
                  or need_to_finalize_.load(std::memory_order_relaxed);
            });
      }

      // Perform a flush of buffered log events
      internal_flush(false, false);

      // If finalization is requested and the event queue is empty,
      // exit the loop
      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }

}  // namespace soralog
