/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_EVENT
#define SORALOG_EVENT

#include <chrono>
#include <cstring>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <soralog/level.hpp>
#include <soralog/sink.hpp>
#include <soralog/util.hpp>

namespace soralog {

  /**
   * @class Event
   * Data of logging event
   */
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
  class Event final {
   public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
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
    template <typename ThreadInfoType, typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    Event(std::string_view name, ThreadInfoType thread_info_type, Level level,
          std::string_view format, const Args &... args)
        : timestamp_(std::chrono::system_clock::now()), level_(level) {
      switch (thread_info_type) {
        case ThreadInfoType::NAME:
          util::getThreadName(thread_name_);
          thread_name_size_ = ::strnlen(thread_name_.data(), 15);
          [[fallthrough]];
        case ThreadInfoType::ID:
          thread_number_ = util::getThreadNumber();
          [[fallthrough]];
        default:
          break;
      }

      try {
        message_size_ =
            fmt::format_to_n(message_.begin(), message_.size(), format, args...)
                .size;
      } catch (const std::exception &exception) {
        message_size_ = fmt::format_to_n(message_.begin(), message_.size(),
                                         "Format error: {}; Format: {}",
                                         exception.what(), format)
                            .size;
        name = "Soralog";
        level_ = Level::ERROR;
      }

      message_size_ = std::min(message_.size(), message_size_);
      name_size_ = std::min(name.size(), name_.size());
      std::copy_n(name.begin(), name_size_, name_.begin());
    }

    /**
     * @returns time when event is happened
     */
    std::chrono::system_clock::time_point timestamp() const noexcept {
      return timestamp_;
    };

    /**
     * @returns number of thread which the event was created in
     */
    size_t thread_number() const noexcept {
      return thread_number_;
    }

    /**
     * @returns name of thread which the event was created in
     */
    std::string_view thread_name() const noexcept {
      return {thread_name_.data(), thread_name_size_};
    }

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
    size_t thread_number_ = 0;
    std::array<char, 16> thread_name_;
    size_t thread_name_size_ = 0;
    std::array<char, 32> name_;
    size_t name_size_;
    Level level_ = Level::OFF;
    std::array<char, 4096> message_;
    size_t message_size_;
  };
}  // namespace soralog

#endif  // SORALOG_EVENT
