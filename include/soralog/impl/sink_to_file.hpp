/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {
  using namespace std::chrono_literals;

  class SinkToFile final : public Sink {
   public:
    SinkToFile() = delete;
    SinkToFile(SinkToFile &&) noexcept = delete;
    SinkToFile(const SinkToFile &) = delete;
    SinkToFile &operator=(SinkToFile &&) noexcept = delete;
    SinkToFile &operator=(const SinkToFile &) = delete;

    SinkToFile(std::string name,
               Level level,
               std::filesystem::path path,
               std::optional<ThreadInfoType> thread_info_type = {},
               std::optional<size_t> capacity = {},
               std::optional<size_t> buffer_size = {},
               std::optional<size_t> max_message_length = {},
               std::optional<size_t> latency = {});
    ~SinkToFile() override;

    void rotate() noexcept override;

    void flush() noexcept override;

   protected:
    void async_flush() noexcept override;

   private:
    void run();

    const std::filesystem::path path_;

    std::unique_ptr<std::thread> sink_worker_{};

    std::vector<char> buff_;
    std::ofstream out_{};
    std::mutex mutex_{};
    std::condition_variable condvar_{};
    std::atomic_bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
    std::atomic_bool need_to_rotate_ = false;
    std::atomic<std::chrono::steady_clock::time_point> next_flush_ =
        std::chrono::steady_clock::time_point();
    std::atomic_flag flush_in_progress_ = false;
  };

}  // namespace soralog
