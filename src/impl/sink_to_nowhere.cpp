/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sink_to_nowhere.hpp"

namespace soralog {

  using namespace std::chrono_literals;

  SinkToNowhere::SinkToNowhere(std::string name)
      : Sink(std::move(name), ThreadInfoType::NONE, 32, sizeof(Event) * 32) {}

  SinkToNowhere::~SinkToNowhere() {
    flush();
  }

  void SinkToNowhere::flush() noexcept {
    while (events_.size() > 0) {
      [[maybe_unused]] auto node = events_.get();
    }
  }

  void SinkToNowhere::rotate() noexcept {
    flush();
  }

}  // namespace soralog
