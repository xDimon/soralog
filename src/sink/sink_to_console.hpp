/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_SINKTOCONSOLE
#define SORALOG_SINKTOCONSOLE

#include <sink.hpp>

#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {

  class SinkToConsole final : public Sink {
   public:
    SinkToConsole() = delete;
    SinkToConsole(SinkToConsole &&) noexcept = delete;
    SinkToConsole(const SinkToConsole &) = delete;
    SinkToConsole &operator=(SinkToConsole &&) noexcept = delete;
    SinkToConsole &operator=(SinkToConsole const &) = delete;

    SinkToConsole(std::string name, bool with_color);
    ~SinkToConsole() override;

    [[nodiscard]] SinkType type() const noexcept override {
      return SinkType::COUT;
    }

    [[nodiscard]] const std::string &name() const noexcept override {
      return name_;
    }

    void flush() noexcept override;

    void rotate() noexcept override{};

   private:
    void run();

    std::string name_;
    bool with_color_;

    std::unique_ptr<std::thread> sink_worker_;

    std::unique_ptr<std::array<char, 50u << 20>> buff_;
    std::mutex mutex_;
    std::condition_variable condvar_;
    bool need_to_finalize_ = false;
    std::atomic_bool need_to_flush_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_SINKTOCONSOLE
