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

  /**
   * @class Multisink
   * @brief A sink that forwards log messages to multiple other sinks.
   *
   * This sink allows log messages to be written to multiple underlying sinks
   * simultaneously. It does not store logs itself but distributes them to
   * the provided sinks.
   */
  class Multisink final : public Sink {
   public:
    Multisink() = delete;
    Multisink(Multisink &&) noexcept = delete;
    Multisink(const Multisink &) = delete;
    Multisink &operator=(Multisink &&) noexcept = delete;
    Multisink &operator=(const Multisink &) = delete;

    /**
     * @brief Constructs a multisink that forwards logs to multiple sinks.
     * @param name Name of the multisink.
     * @param level Minimum logging level.
     * @param sinks A list of sinks to forward log messages to.
     */
    Multisink(std::string name,
              Level level,
              std::vector<std::shared_ptr<Sink>> sinks);

    /**
     * @brief Rotates log files for all underlying sinks.
     *
     * Calls `rotate()` on each underlying sink, which is relevant for
     * file-based sinks to switch to a new log file.
     */
    void rotate() noexcept override;

    /**
     * @brief Flushes all underlying sinks.
     *
     * This method first calls `async_flush()` on each sink to trigger
     * asynchronous flushing, then calls `flush()` to ensure all pending logs
     * are written.
     */
    void flush() noexcept override;

   private:
    /**
     * @brief Asynchronous flushing is not supported for multisinks.
     */
    void async_flush() noexcept override {};
  };

}  // namespace soralog
