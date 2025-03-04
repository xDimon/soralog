/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include <soralog/sink.hpp>

namespace soralog {

  /**
   * @class SinkMock
   * @brief Mock implementation of the Sink class for testing.
   *
   * This class overrides Sink methods to intercept logging calls and enable
   * their verification in tests using Google Mock. It does not perform
   * actual logging, but allows checking that methods are called with
   * expected parameters.
   */
  class SinkMock : public Sink {
    std::string name_;

   public:
    /**
     * @brief Constructs a mock sink with minimal buffer and disabled fault
     * reactions.
     *
     * @param name The name of the mock sink.
     */
    SinkMock(std::string name)
        : Sink(std::move(name),
               Level::TRACE,
               ThreadInfoType::NONE,
               4,                               // Small event queue for testing
               1024,                            // Max message length
               4096,                            // Buffer size
               0,                               // No latency
               AtFaultReactionType::IGNORE) {}  // Ignore faults in tests

    ~SinkMock() override = default;

    /**
     * @brief Mocks the push method to capture logging events.
     *
     * Unlike a real sink, this method does not format or store messages.
     * Instead, it calls `mocked_push` to allow validation in tests.
     *
     * @param name Logger name.
     * @param level Logging level.
     * @param format Log message format.
     * @param max_message_length Maximum length of the message.
     * @param args Additional arguments (not used in mock).
     */
    template <typename... Args>
    void push(std::string_view name,
              Level level,
              std::string_view format,
              size_t max_message_length,
              const Args &...args) noexcept(IF_RELEASE) {
      mocked_push(name, level, format);
    }

    /// Mock method for verifying calls to push().
    MOCK_METHOD(void,
                mocked_push,
                (std::string_view, Level, std::string_view),
                (const));

    /**
     * @brief Mocks the flush method.
     *
     * Instead of flushing logs, this method calls `mocked_flush` to
     * track calls in tests.
     */
    void flush() noexcept override {
      mocked_flush();
    }

    /// Mock method for verifying calls to flush().
    MOCK_METHOD(void, mocked_flush, (), (const));

    /**
     * @brief Mocks the async_flush method.
     *
     * This does not perform asynchronous flushing, but instead calls
     * `mocked_async_flush` to allow verification in tests.
     */
    void async_flush() noexcept override {
      mocked_async_flush();
    }

    /// Mock method for verifying calls to async_flush().
    MOCK_METHOD(void, mocked_async_flush, (), (const));

    /**
     * @brief Mocks the rotate method.
     *
     * This does not rotate logs but calls `mocked_rotate` for test tracking.
     */
    void rotate() noexcept override {
      mocked_rotate();
    }

    /// Mock method for verifying calls to rotate().
    MOCK_METHOD(void, mocked_rotate, (), (const));
  };

}  // namespace soralog
