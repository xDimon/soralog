/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_nowhere.hpp>

namespace soralog {

  using namespace std::chrono_literals;

  SinkToNowhere::SinkToNowhere(std::string name)
      : Sink(std::move(name), ThreadInfoType::NONE, 32, sizeof(Event) * 32, 0) {
  }

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
