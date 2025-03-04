/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/sink_to_nowhere.hpp>

namespace soralog {

  using namespace std::chrono_literals;

  // Constructor initializes a "black hole" sink that discards all logs
  SinkToNowhere::SinkToNowhere(std::string name)
      : Sink(std::move(name),
             Level::OFF,  // No logging level, effectively disabling logging
             ThreadInfoType::NONE,  // No thread info required
             1024,                  // Event queue capacity (not really used)
             0,     // Max message length (irrelevant as messages are discarded)
             0,     // Buffer size (irrelevant)
             1000,  // Latency in milliseconds (not relevant for this sink)
             AtFaultReactionType::IGNORE) {}  // Ignore errors

  // Destructor ensures any remaining events are cleared before destruction
  SinkToNowhere::~SinkToNowhere() {
    flush();
  }

  // Flushes the event queue by discarding all stored events
  void SinkToNowhere::flush() noexcept {
    while (events_.size() > 0) {
      std::ignore = events_.get();  // Discard each event
    }
  }

  // Async flush is identical to flush, as there is no background processing
  void SinkToNowhere::async_flush() noexcept {
    flush();
  }

  // Rotation is a no-op, but ensures that any pending events are discarded
  void SinkToNowhere::rotate() noexcept {
    flush();
  }

}  // namespace soralog
