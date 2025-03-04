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
     * @brief Flushes the sink (no-op).
     */
    void flush() noexcept override;

    /**
     * @brief Asynchronously flushes the sink (no-op).
     */
    void async_flush() noexcept override;

    /**
     * @brief Rotates the sink (no-op).
     */
    void rotate() noexcept override;
  };

}  // namespace soralog
