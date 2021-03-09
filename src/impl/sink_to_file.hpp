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
               std::string filename);
    ~SinkToFile() override;

    [[nodiscard]] const std::string &name() const noexcept override {
      return name_;
    }

    void flush() noexcept override;

    void rotate() noexcept override;

   private:
    void run();

    std::string name_{};
    std::filesystem::path path_{};
    std::unique_ptr<std::thread> sink_worker_{};

    std::unique_ptr<std::array<char, 50u << 20>> buff_;
    std::ofstream out_{};
    std::mutex mutex_{};
    std::condition_variable condvar_{};
    bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
    std::atomic_bool need_to_rotate_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_SINKTOFILE
