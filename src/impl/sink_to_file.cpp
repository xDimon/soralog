/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sink_to_file.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

#include <fmt/chrono.h>

namespace soralog {

  namespace {

    using namespace std::chrono_literals;

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
                         std::string filename, ThreadFlag thread_flag,
                         size_t events_capacity, size_t buffer_size,
                         size_t latency_ms)
      : Sink(std::move(name), thread_flag, events_capacity, buffer_size),
        path_(std::move(path).string() + "/" + std::move(filename)),
        buffer_size_(buffer_size),
        latency_(latency_ms),
        buff_(buffer_size_) {
    sink_worker_ = std::make_unique<std::thread>([this] { run(); });
    out_.open(path_, std::ios::app);
    if (not out_.is_open()) {
      std::cerr << "Can't open log file '" << path_ << "': " << strerror(errno)
                << std::endl;
    }
  }

  SinkToFile::~SinkToFile() {
    need_to_finalize_ = true;
    flush();
    sink_worker_->join();
    sink_worker_.reset();
  }

  void SinkToFile::flush() noexcept {
    need_to_flush_ = true;
    condvar_.notify_one();
  }

  void SinkToFile::rotate() noexcept {
    need_to_rotate_ = true;
    flush();
  }

  void SinkToFile::run() {
    util::setThreadName("log:" + name_);

    auto next_flush = std::chrono::steady_clock::now();

    while (true) {
      if (events_.size() == 0) {
        if (need_to_finalize_) {
          return;
        }

        bool true_v = true;
        if (need_to_rotate_.compare_exchange_weak(true_v, false)) {
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
      }

      std::unique_lock lock(mutex_);
      if (not condvar_.wait_for(lock, std::chrono::milliseconds(100),
                                [this] { return events_.size() > 0; })) {
        continue;
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
          ptr = ptr + datetime.size();  // NOLINT

          put_separator(ptr);

          // Thread

          switch (thread_flag_) {
            case ThreadFlag::NAME: {
              put_string(ptr, event.thread_name(), 15);
              put_separator(ptr);
              break;
            }

            case ThreadFlag::ID: {
              ptr = fmt::format_to_n(ptr, end - ptr, "T:{:<6}",
                                     event.thread_number())
                        .out;
              put_separator(ptr);
              break;
            }

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
            or std::chrono::steady_clock::now() >= next_flush) {
          out_.write(begin, ptr - begin);
          ptr = begin;
        }

        if (not node) {
          bool true_v = true;
          if (need_to_flush_.compare_exchange_weak(true_v, false)) {
            out_.flush();
          }
          if (need_to_finalize_) {
            return;
          }
          break;
        }
      }
    }
  }
}  // namespace soralog
