/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/multisink.hpp>

#include <iostream>

#include <fmt/chrono.h>

namespace soralog {

  Multisink::Multisink(std::string name,
                       std::vector<std::shared_ptr<Sink>> sinks)
      : Sink(std::move(name), std::move(sinks)) {}

  void Multisink::flush() noexcept {
    for (const auto &sink : underlying_sinks_) {
      sink->async_flush();
    }
    for (const auto &sink : underlying_sinks_) {
      sink->flush();
    }
  }

  void Multisink::rotate() noexcept {
    for (const auto &sink : underlying_sinks_) {
      sink->rotate();
    }
  }

}  // namespace soralog
