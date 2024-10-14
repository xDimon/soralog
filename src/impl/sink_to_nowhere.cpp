/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_nowhere.hpp>

namespace soralog {

  using namespace std::chrono_literals;

  SinkToNowhere::SinkToNowhere(std::string name)
      : Sink(std::move(name),
             Level::OFF,
             ThreadInfoType::NONE,
             1024,
             0,
             0,
             1000) {}

  SinkToNowhere::~SinkToNowhere() {
    flush();
  }

  void SinkToNowhere::flush() noexcept {
    while (events_.size() > 0) {
      std::ignore = events_.get();
    }
  }

  void SinkToNowhere::async_flush() noexcept {
    flush();
  }

  void SinkToNowhere::rotate() noexcept {
    flush();
  }

}  // namespace soralog
