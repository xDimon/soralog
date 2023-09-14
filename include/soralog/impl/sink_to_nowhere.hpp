/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

namespace soralog {
  using namespace std::chrono_literals;

  class SinkToNowhere final : public Sink {
   public:
    SinkToNowhere() = delete;
    SinkToNowhere(SinkToNowhere &&) noexcept = delete;
    SinkToNowhere(const SinkToNowhere &) = delete;
    SinkToNowhere &operator=(SinkToNowhere &&) noexcept = delete;
    SinkToNowhere &operator=(SinkToNowhere const &) = delete;

    explicit SinkToNowhere(std::string name);
    ~SinkToNowhere() override;

    void flush() noexcept override;

    void async_flush() noexcept override;

    void rotate() noexcept override;
  };

}  // namespace soralog
