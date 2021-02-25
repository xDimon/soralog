/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_EVENT
#define SORALOG_EVENT

#include <chrono>
#include <string_view>

#include <fmt/format.h>

#include <log_levels.hpp>

namespace soralog {
  class Event final {
   public:
    Event() = default;
    Event(Event &&) noexcept = delete;
    Event(const Event &) = delete;
    ~Event() = default;
    Event &operator=(Event &&) noexcept = delete;
    Event &operator=(Event const &) = delete;

    template <typename... Args>
    Event(std::string_view name, Level level, std::string_view format,
          Args &&... args)
        : time(std::chrono::system_clock::now()), name(name), level(level) {
      auto result = fmt::format_to_n(message.begin(), message.size(), format,
                                     std::forward<Args>(args)...);
      size = result.size;
    }

    std::chrono::system_clock::time_point time;
    std::string_view name;
    Level level = Level::OFF;
    std::array<char, 4096> message;
    size_t size;
  };
}  // namespace soralog

#endif  // SORALOG_EVENT
