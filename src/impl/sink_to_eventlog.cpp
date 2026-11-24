/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_eventlog.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <system_error>
#include <thread>

#include <soralog/internals.hpp>

// After soralog headers: windows.h defines macros named like Level values
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef ERROR
#undef IGNORE

namespace soralog {

  namespace {

    // Separator used between logical parts of a log record.
    constexpr std::string_view separator = "  ";

    // Longest insertion string the Event Log takes, in UTF-16 code units
    constexpr size_t max_string_length = 31839;

    void put_separator(char *&ptr) {
      for (auto c : separator) {
        *ptr++ = c;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      }
    }

    /**
     * @brief Writes a dot and the microseconds as six digits, zero-padded.
     * @param ptr Reference to the pointer where the digits should be written.
     * @param usec Microseconds, below 1000000.
     */
    void put_microseconds(char *&ptr, uint32_t usec) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      *ptr++ = '.';
      for (int i = 5; i >= 0; --i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ptr[i] = static_cast<char>('0' + (usec % 10));
        usec /= 10;
      }
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ptr += 6;
    }

    void put_level(char *&ptr, Level level) {
      const char *str = levelToStr(level);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      while (auto c = *str++) {
        *ptr++ = c;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      }
    }

    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        *ptr++ = c;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      }
    }

    /// Record type of a level; 0 if the level is not logged
    WORD recordType(Level level) {
      switch (level) {
        case Level::FATAL:
        case Level::CRITICAL:
        case Level::ERROR:
          return EVENTLOG_ERROR_TYPE;
        case Level::WARN:
          return EVENTLOG_WARNING_TYPE;
        case Level::INFO:
        case Level::VERBOSE:
        case Level::DEBUG:
          return EVENTLOG_INFORMATION_TYPE;
        default:  // OFF, TRACE, IGNORE
          return 0;
      }
    }

    /// UTF-16 from UTF-8; invalid sequences become U+FFFD
    std::wstring toWide(std::string_view str) {
      std::wstring result;
      if (str.empty()) {
        return result;
      }
      const auto size = MultiByteToWideChar(
          CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
      result.resize(static_cast<size_t>(size));
      MultiByteToWideChar(CP_UTF8,
                          0,
                          str.data(),
                          static_cast<int>(str.size()),
                          result.data(),
                          size);
      return result;
    }

  }  // namespace

  SinkToEventLog::SinkToEventLog(std::string name,
                                 Level level,
                                 std::string source,
                                 std::optional<ThreadInfoType> thread_info_type,
                                 std::optional<size_t> capacity,
                                 std::optional<size_t> max_message_length,
                                 std::optional<size_t> buffer_size,
                                 std::optional<size_t> latency,
                                 std::optional<AtFaultReactionType> at_fault,
                                 std::optional<uint32_t> event_id)
      : Sink(std::move(name),
             level,
             thread_info_type.value_or(ThreadInfoType::NONE),
             capacity.value_or(1u << 11),            // Default: 2048 events
             max_message_length.value_or(1u << 10),  // Default: 1024 bytes
             buffer_size.value_or(1u << 22),         // Default: 4 MB
             latency.value_or(1000),                 // Default: 1 second
             at_fault.value_or(AtFaultReactionType::IGNORE)),
        source_(std::move(source)),
        event_id_(event_id.value_or(0)),
        buff_(max_buffer_size_),
        wide_buff_(max_string_length + 1) {
    // An unregistered source is accepted: records go to the Application log
    handle_ = RegisterEventSourceW(nullptr, toWide(source_).c_str());
    if (handle_ == nullptr) {
      throw std::system_error(static_cast<int>(GetLastError()),
                              std::system_category(),
                              "SinkToEventLog has not been created: can't open "
                              "event source '"
                                  + source_ + "'");
    }

    if (latency_ != std::chrono::milliseconds::zero()) {
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToEventLog::~SinkToEventLog() {
    if (latency_ != std::chrono::milliseconds::zero()) {
      {
        // Under the lock: the worker checks the flags before it falls asleep
        std::lock_guard lock(mutex_);
        need_to_finalize_.store(true, std::memory_order_release);
      }
      condvar_.notify_one();
      if (sink_worker_ and sink_worker_->joinable()) {
        sink_worker_->join();
        sink_worker_.reset();
      }
    } else {
      sync_flush();
    }
    DeregisterEventSource(handle_);
  }

  void SinkToEventLog::flush() noexcept {
    sync_flush();
  }

  void SinkToEventLog::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      // Wake the worker only when the request appears: the flag is reset by
      // the flush draining the queue, until then repeated requests are no-ops
      if (not need_to_flush_.load(std::memory_order_relaxed)
          and not need_to_flush_.exchange(true, std::memory_order_acq_rel)) {
        // Under the lock: the worker checks the flags before it falls asleep
        std::lock_guard lock(mutex_);
        condvar_.notify_one();
      }
    } else {
      internal_flush();
    }
  }

  void SinkToEventLog::sync_flush() noexcept {
    need_to_flush_.store(true, std::memory_order_release);
    internal_flush(true);
  }

  void SinkToEventLog::internal_flush(bool wait, bool bounded) noexcept {
    // Runs a flush; called with flush_in_progress_ set
    auto flush_once = [this, bounded] {
      const auto number =
          flushes_started_.fetch_add(1, std::memory_order_relaxed) + 1;
      // Pairs with the fence in the waiting branch: either the waiter sees
      // this flush started, or this flush sees the events of the waiter
      std::atomic_thread_fence(std::memory_order_seq_cst);
      flush_events(bounded ? events_.size() : events_.capacity());
      flushes_done_.store(number, std::memory_order_release);
    };
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
      flush_once();
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
        flush_once();
        flush_in_progress_.clear(std::memory_order_release);
        // An event published while the flag was held may have been missed
        // both by flush_events() and by its own producer, which found the flag
        // set. The worker takes it; a bounded flush leaves it to the producer,
        // which flushes synchronously after queueing anyway
        std::atomic_thread_fence(std::memory_order_seq_cst);
      } while (not bounded and events_.has_ready());
    }
  }

  void SinkToEventLog::flush_events(size_t limit) noexcept {
    // Move the deadline, otherwise the idle worker would wake up immediately
    // again and again
    next_flush_.store(std::chrono::steady_clock::now() + latency_,
                      std::memory_order_release);

    auto *const begin = buff_.data();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *const end = buff_.data() + buff_.size();
    auto *ptr = begin;

    // The second of the previous event and its formatted date are kept
    // between calls: they change once a second, and the worker calls this
    // function much more often. Safe without a lock: only one thread runs
    // this function at a time, under flush_in_progress_
    auto &psec = cached_sec_;
    auto &datetime = cached_datetime_;
    std::tm tm{};

    while (true) {
      auto node = take_event(limit);
      if (not node) {
        need_to_flush_.store(false, std::memory_order_release);
        break;
      }
      const auto &event = *node;

      const auto type = recordType(event.level());
      if (type == 0) {
        continue;
      }

      ptr = begin;

      // Timestamp: the record has its own, but in seconds only
      const auto time = event.timestamp().time_since_epoch();
      const auto sec = time / 1s;
      const auto usec = time % 1s / 1us;
      if (psec != sec) {
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
      std::memcpy(ptr, datetime.data(), datetime.size());
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ptr = ptr + datetime.size();
      put_microseconds(ptr, static_cast<uint32_t>(usec));
      put_separator(ptr);

      // Thread information
      switch (thread_info_type_) {
        case ThreadInfoType::NAME:
          put_string(ptr, event.thread_name());
          put_separator(ptr);
          break;

        case ThreadInfoType::ID:
          ptr = fmt::format_to_n(ptr, end - ptr, "T:{}", event.thread_number())
                    .out;
          put_separator(ptr);
          break;

        default:
          break;
      }

      put_level(ptr, event.level());
      put_separator(ptr);
      put_string(ptr, event.name());
      put_separator(ptr);
      put_string(ptr, event.message());

      // UTF-16 units are never more than UTF-8 bytes: cutting the bytes keeps
      // the string within the limit; a cut sequence becomes U+FFFD
      const auto size =
          std::min(static_cast<size_t>(ptr - begin), max_string_length);
      const auto wide_size = MultiByteToWideChar(CP_UTF8,
                                                 0,
                                                 begin,
                                                 static_cast<int>(size),
                                                 wide_buff_.data(),
                                                 static_cast<int>(size));
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      wide_buff_[static_cast<size_t>(std::max(wide_size, 0))] = L'\0';

      std::array<LPCWSTR, 1> strings{wide_buff_.data()};
      // Failure is ignored, as by the syslog sink: no way to report it
      ReportEventW(static_cast<HANDLE>(handle_),
                   type,
                   0,
                   event_id_,
                   nullptr,
                   static_cast<WORD>(strings.size()),
                   0,
                   strings.data(),
                   nullptr);
    }
  }

  void SinkToEventLog::run() {
    // Set the thread name for debugging purposes
    util::setThreadName("log:" + name_);

    // Initialize the next flush time to the current moment
    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);
        // Wait until the next scheduled flush time or until notified
        condvar_.wait_until(
            lock, next_flush_.load(std::memory_order_relaxed), [this] {
              return need_to_flush_.load(std::memory_order_relaxed)
                  or need_to_finalize_.load(std::memory_order_relaxed);
            });
      }

      internal_flush(false, false);

      // If finalization is requested and there are no pending events, exit
      if (need_to_finalize_.load(std::memory_order_acquire)
          and events_.size() == 0) {
        return;
      }
    }
  }

}  // namespace soralog
