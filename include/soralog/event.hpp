/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_EVENT
#define SORALOG_EVENT

#include <chrono>
#include <string_view>

#include <fmt/format.h>

#include <soralog/level.hpp>

namespace soralog {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
  class Event final {
   public:
    Event() = default;
    Event(Event &&) noexcept = delete;
    Event(const Event &) = delete;
    ~Event() = default;
    Event &operator=(Event &&) noexcept = delete;
    Event &operator=(Event const &) = delete;

    template <typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    Event(std::string_view name, Level level, std::string_view format,
          const Args &... args)
        : time(std::chrono::system_clock::now()),
          name(name.data(), std::min(name.size(), 15ul)),
          level(level) {
      auto result =
          fmt::format_to_n(message.begin(), message.size(), format, args...);
      size = result.size;
    }

    std::chrono::system_clock::time_point time;
    const std::string name;
    Level level = Level::OFF;
    std::array<char, 4096> message;
    size_t size = 0;
  };
}  // namespace soralog

#endif  // SORALOG_EVENT
