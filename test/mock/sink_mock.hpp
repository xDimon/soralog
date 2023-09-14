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

  class SinkMock : public Sink {
    std::string name_;

   public:
    SinkMock(std::string name)
        : Sink(std::move(name), ThreadInfoType::NONE, 4, 1024, 4096, 0) {}
    ~SinkMock() override = default;

    template <typename... Args>
    void push(std::string_view name, Level level, std::string_view format,
              size_t max_message_length,
              const Args &...args) noexcept(IF_RELEASE) {
      mocked_push(name, level, format);
    }
    MOCK_METHOD3(mocked_push, void(std::string_view, Level, std::string_view));

    void flush() noexcept override {
      mocked_flush();
    }
    MOCK_METHOD0(mocked_flush, void());

    void async_flush() noexcept override {
      mocked_async_flush();
    }
    MOCK_METHOD0(mocked_async_flush, void());

    void rotate() noexcept override {
      mocked_rotate();
    }
    MOCK_METHOD0(mocked_rotate, void());
  };

}  // namespace soralog
