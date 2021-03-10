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
  using namespace std::chrono_literals;

  SinkToFile::SinkToFile(std::string name, std::filesystem::path path,
                         std::string filename)
      : name_(std::move(name)),
        path_(std::move(path).string() + "/" + std::move(filename)) {
    buff_ = std::make_unique<decltype(buff_)::element_type>();
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
    auto next_flush = std::chrono::steady_clock::now();

    while (true) {
      if (events_->size() == 0) {
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
                                [this] { return events_->size() > 0; })) {
        continue;
      }

      auto *const begin = buff_->data();
      auto *const end = buff_->data() + buff_->size();
      auto *ptr = begin;

      decltype(1s / 1s) psec = 0;
      std::tm tm{};
      std::array<char, 18> datetime{};  // "00.00.00 00:00:00."

      while (true) {
        auto node = events_->get();
        if (node) {
          const auto &event = *node;

          const auto time = event.time.time_since_epoch();
          const auto sec = time / 1s;
          const auto usec = time % 1s / 1us;

          if (psec != sec) {
            tm = fmt::localtime(sec);
            fmt::format_to_n(datetime.data(), datetime.size(),
                             "{:0>2}.{:0>2}.{:0>2} {:0>2}:{:0>2}:{:0>2}.",
                             tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday,
                             tm.tm_hour, tm.tm_min, tm.tm_sec);
            psec = sec;
          }

          std::memcpy(ptr, datetime.data(), datetime.size());
          ptr = ptr + datetime.size();  // NOLINT

          ptr = fmt::format_to_n(ptr, end - ptr, "{:0>6}  {: <8}  {}  ", usec,
                                 levelToStr(event.level), event.name)
                    .out;

          std::memcpy(ptr, event.message.data(), event.size);
          ptr = ptr + event.size;  // NOLINT
          *ptr++ = '\n';           // NOLINT

          size_ -= event.size;
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
