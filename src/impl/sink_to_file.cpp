/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_file.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>

#include <fmt/chrono.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

    // Separator is using between logical parts of log record.
    // Might be any substring or symbol: space, tab, etc.
    // Couple of space is selected to differ of single space
    constexpr std::string_view separator = "  ";

    void put_separator(char *&ptr) {
      for (auto c : separator) {
        *ptr++ = c;  // NOLINT
      }
    }

    void put_level(char *&ptr, Level level) {
      const char *const end = ptr + 8;  // NOLINT
      const char *str = levelToStr(level);
      do {
        *ptr++ = *str++;  // NOLINT
      } while (*str != '\0');
      while (ptr < end) {
        *ptr++ = ' ';  // NOLINT
      }
    }

    void put_level_short(char *&ptr, Level level) {
      *ptr++ = levelToChar(level);  // NOLINT
    }

    template <typename T>
    void put_string(char *&ptr, const T &name) {
      for (auto c : name) {
        *ptr++ = c;  // NOLINT
      }
    }

    template <typename T>
    void put_string(char *&ptr, const T &name, size_t width) {
      if (width == 0)
        return;
      for (auto c : name) {
        if (c == '\0' or width == 0)
          break;
        *ptr++ = c;  // NOLINT
        --width;
      }
      while (width--) *ptr++ = ' ';  // NOLINT
    }

  }  // namespace

  SinkToFile::SinkToFile(std::string name, std::filesystem::path path,
                         std::optional<ThreadInfoType> thread_info_type,
                         std::optional<size_t> capacity,
                         std::optional<size_t> buffer_size,
                         std::optional<size_t> latency)
      : Sink(std::move(name), thread_info_type.value_or(ThreadInfoType::NONE),
             capacity.value_or(1u << 11),     // 2048 events
             buffer_size.value_or(1u << 22),  // 4 Mb
             latency.value_or(1000)),         // 1 sec
        path_(std::move(path)),
        buff_(max_buffer_size_) {
    out_.open(path_, std::ios::app);
    if (not out_.is_open()) {
      std::cerr << "Can't open log file '" << path_ << "': " << strerror(errno)
                << std::endl;
    } else if (latency_ != std::chrono::milliseconds::zero()) {
      sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    }
  }

  SinkToFile::~SinkToFile() {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_finalize_.store(true, std::memory_order_release);
      async_flush();
      sink_worker_->join();
      sink_worker_.reset();
    } else {
      flush();
    }
  }

  void SinkToFile::async_flush() noexcept {
    if (latency_ != std::chrono::milliseconds::zero()) {
      need_to_flush_.store(true, std::memory_order_release);
      condvar_.notify_one();
    } else {
      flush();
    }
  }

  void SinkToFile::flush() noexcept {
    bool false_v = false;
    if (not flush_in_progress_.compare_exchange_strong(
            false_v, true, std::memory_order_acq_rel)) {
      return;
    }

    auto *const begin = buff_.data();
    auto *const end = buff_.data() + buff_.size();  // NOLINT
    auto *ptr = begin;

    decltype(1s / 1s) psec = 0;
    std::tm tm{};
    std::array<char, 17> datetime{};  // "00.00.00 00:00:00"

    while (true) {
      auto node = events_.get();
      if (node) {
        const auto &event = *node;

        const auto time = event.timestamp().time_since_epoch();
        const auto sec = time / 1s;
        const auto usec = time % 1s / 1us;

        if (psec != sec) {
          tm = fmt::localtime(sec);
          fmt::format_to_n(datetime.data(), datetime.size(),
                           "{:0>2}.{:0>2}.{:0>2} {:0>2}:{:0>2}:{:0>2}",
                           tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
                           tm.tm_hour, tm.tm_min, tm.tm_sec);
          psec = sec;
        }

        // Timestamp

        std::memcpy(ptr, datetime.data(), datetime.size());
        ptr = ptr + datetime.size();  // NOLINT

        ptr = fmt::format_to_n(ptr, end - ptr, ".{:0>6}", usec).out;

        put_separator(ptr);

        // Thread

        switch (thread_info_type_) {
          case ThreadInfoType::NAME:
            put_string(ptr, event.thread_name(), 15);
            put_separator(ptr);
            break;

          case ThreadInfoType::ID:
            ptr = fmt::format_to_n(ptr, end - ptr, "T:{:<6}",
                                   event.thread_number())
                      .out;
            put_separator(ptr);
            break;

          default:
            break;
        }

        // Level

        put_level(ptr, event.level());
        put_separator(ptr);

        // Name

        put_string(ptr, event.name());
        put_separator(ptr);

        // Message

        put_string(ptr, event.message());
        *ptr++ = '\n';  // NOLINT

        size_ -= event.message().size();
      }

      if ((end - ptr) < sizeof(Event) or not node
          or std::chrono::steady_clock::now()
              >= next_flush_.load(std::memory_order_acquire)) {
        next_flush_.store(std::chrono::steady_clock::now() + latency_,
                          std::memory_order_release);
        out_.write(begin, ptr - begin);
        ptr = begin;
      }

      if (not node) {
        bool true_v = true;
        if (need_to_flush_.compare_exchange_weak(true_v, false,
                                                 std::memory_order_acq_rel)) {
          out_.flush();
        }
        break;
      }
    }

    bool true_v = true;
    if (need_to_rotate_.compare_exchange_weak(true_v, false,
                                              std::memory_order_acq_rel)) {
      std::ofstream out;
      out.open(path_, std::ios::app);
      if (not out.is_open()) {
        if (out_.is_open()) {
          std::cerr << "Can't re-open log file '" << path_
                    << "': " << strerror(errno) << std::endl;
        } else {
          std::cerr << "Can't open log file '" << path_
                    << "': " << strerror(errno) << std::endl;
        }
      } else {
        std::swap(out_, out);
      }
    }

    flush_in_progress_.store(false, std::memory_order_release);
  }

  void SinkToFile::rotate() noexcept {
    need_to_rotate_.store(true, std::memory_order_release);
    async_flush();
  }

  void SinkToFile::run() {
    util::setThreadName("log:" + name_);

    next_flush_.store(std::chrono::steady_clock::now(),
                      std::memory_order_relaxed);

    while (true) {
      {
        std::unique_lock lock(mutex_);
        if (condvar_.wait_until(lock,
                                next_flush_.load(std::memory_order_relaxed))
            == std::cv_status::no_timeout) {
          if (not need_to_flush_.load(std::memory_order_relaxed)
              and not need_to_finalize_.load(std::memory_order_relaxed)) {
            continue;
          }
        }
      }

      flush();

      if (need_to_finalize_.load(std::memory_order_acquire)
          && events_.size() == 0) {
        return;
      }
    }
  }
}  // namespace soralog
