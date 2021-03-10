/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_SINKTOCONSOLE
#define SORALOG_SINKTOCONSOLE

#include <soralog/sink.hpp>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {
  using namespace std::chrono_literals;

  class SinkToConsole final : public Sink {
   public:
    SinkToConsole() = delete;
    SinkToConsole(SinkToConsole &&) noexcept = delete;
    SinkToConsole(const SinkToConsole &) = delete;
    SinkToConsole &operator=(SinkToConsole &&) noexcept = delete;
    SinkToConsole &operator=(SinkToConsole const &) = delete;

    SinkToConsole(std::string name, bool with_color);
    ~SinkToConsole() override;

    [[nodiscard]] const std::string &name() const noexcept override {
      return name_;
    }

    void flush() noexcept override;

    void rotate() noexcept override{};

   private:
    void run();

    std::string name_;
    bool with_color_;
    std::chrono::milliseconds latency_ = 250ms;

    std::unique_ptr<std::thread> sink_worker_;

    std::unique_ptr<std::array<char, 128u << 10>> buff_;
    std::mutex mutex_;
    std::condition_variable condvar_;
    bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_SINKTOCONSOLE
