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

    SinkToConsole(std::string name, bool with_color,
                  ThreadInfoType thread_info_type = ThreadInfoType::NONE,
                  size_t events_capacity = 1u << 6,  // 64 events
                  size_t buffer_size = 1u << 17,     // 128 Kb
                  size_t latency_ms = 200);          // 200 ms
    ~SinkToConsole() override;

    void flush() noexcept override;

    void rotate() noexcept override{};

   private:
    void run();

    bool with_color_ = false;
    const size_t buffer_size_ = 1 << 17;            // 128Kb
    const std::chrono::milliseconds latency_{200};  // 200ms

    std::unique_ptr<std::thread> sink_worker_;

    std::vector<char> buff_;
    std::mutex mutex_;
    std::condition_variable condvar_;
    bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_SINKTOCONSOLE
