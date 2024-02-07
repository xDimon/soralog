/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/sink.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace soralog {
  using namespace std::chrono_literals;

  class Multisink final : public Sink {
   public:
    Multisink() = delete;
    Multisink(Multisink &&) noexcept = delete;
    Multisink(const Multisink &) = delete;
    Multisink &operator=(Multisink &&) noexcept = delete;
    Multisink &operator=(Multisink const &) = delete;

    Multisink(std::string name, std::vector<std::shared_ptr<Sink>> sinks);

    void rotate() noexcept override;
    void flush() noexcept override;

   private:
    void async_flush() noexcept override{};
  };

}  // namespace soralog
