/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_EVENT
#define SORALOG_EVENT

#include <chrono>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <soralog/level.hpp>

namespace soralog {

  /**
   * @class Event
   * Data of logging event
   */
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
  class Event final {
   public:
    Event() = default;
    Event(Event &&) noexcept = delete;
    Event(const Event &) = delete;
    ~Event() = default;
    Event &operator=(Event &&) noexcept = delete;
    Event &operator=(Event const &) = delete;

    /**
     * @param name of logger
     * @param level of event
     * @param format and @param args defines message of event
     */
    template <typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    Event(std::string_view name, Level level, std::string_view format,
          const Args &... args)
        : timestamp_(std::chrono::system_clock::now()), level_(level) {
      name_size_ = std::min(name.size(), name_.size());
      std::copy_n(name.begin(), name_size_, name_.begin());
      auto result =
          fmt::format_to_n(message_.begin(), message_.size(), format, args...);
      message_size_ = result.size;
    }

    /**
     * @returns time when event is happened
     */
    std::chrono::system_clock::time_point timestamp() const noexcept {
      return timestamp_;
    };

    /**
     * @returns name of logger through which the event was created
     */
    std::string_view name() const noexcept {
      return {name_.data(), name_size_};
    }

    /**
     * @returns level of event
     */
    Level level() const noexcept {
      return level_;
    }

    /**
     * @returns message of event
     */
    std::string_view message() const noexcept {
      return {message_.data(), message_size_};
    }

   private:
    std::chrono::system_clock::time_point timestamp_;
    std::array<char, 32> name_;
    size_t name_size_;
    Level level_ = Level::OFF;
    std::array<char, 4096> message_;
    size_t message_size_ = 0;
  };
}  // namespace soralog

#endif  // SORALOG_EVENT
