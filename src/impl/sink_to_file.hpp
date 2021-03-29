/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_SINKTOFILE
#define SORALOG_SINKTOFILE

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
    SinkToFile &operator=(SinkToFile const &) = delete;

    SinkToFile(std::string name, std::filesystem::path path,
               ThreadInfoType thread_info_type = ThreadInfoType::NONE,
               size_t events_capacity = 1u << 11,  // 2048 events
               size_t buffer_size = 1u << 22,      // 4 Mb
               size_t latency_ms = 1000);          // 1 sec
    ~SinkToFile() override;

    void flush() noexcept override;

    void rotate() noexcept override;

   private:
    void run();

    const std::filesystem::path path_{};
    const size_t buffer_size_ = 1 << 22;             // 4Mb
    const std::chrono::milliseconds latency_{1000};  // 1sec

    std::unique_ptr<std::thread> sink_worker_{};

    std::vector<char> buff_;
    std::ofstream out_{};
    std::mutex mutex_{};
    std::condition_variable condvar_{};
    std::atomic_bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
    std::atomic_bool need_to_rotate_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_SINKTOFILE
