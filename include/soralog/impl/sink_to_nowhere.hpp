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

  /**
   * @class SinkToNowhere
   * @brief A sink that discards all log messages.
   *
   * This sink is useful for disabling logging in certain configurations
   * without modifying the logging system. It implements all required sink
   * methods but does nothing.
   */
  class SinkToNowhere final : public Sink {
   public:
    SinkToNowhere() = delete;
    SinkToNowhere(SinkToNowhere &&) noexcept = delete;
    SinkToNowhere(const SinkToNowhere &) = delete;
    SinkToNowhere &operator=(SinkToNowhere &&) noexcept = delete;
    SinkToNowhere &operator=(const SinkToNowhere &) = delete;

    /**
     * @brief Constructs a sink that discards all log messages.
     * @param name Name of the sink.
     */
    explicit SinkToNowhere(std::string name);

    /**
     * @brief Destroys the sink.
     */
    ~SinkToNowhere() override;

    /**
     * @brief Discards queued events.
     */
    void flush() noexcept override;

    /**
     * @brief Discards queued events.
     */
    void async_flush() noexcept override;

    /**
     * @brief Discards queued events.
     */
    void sync_flush() noexcept override;

    /**
     * @brief Discards queued events; there is nothing to rotate.
     */
    void rotate() noexcept override;
  };

}  // namespace soralog
