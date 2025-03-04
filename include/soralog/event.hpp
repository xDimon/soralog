/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <string_view>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <soralog/level.hpp>
#include <soralog/sink.hpp>
#include <soralog/util.hpp>

/**
 * @brief Defines the Event class, which represents a single logging event.
 *
 * The `Event` class stores information about a log entry, including its
 * timestamp, severity level, associated logger name, thread information,
 * and the formatted log message. The class is optimized for fast log
 * creation and retrieval.
 */

namespace soralog {

  /**
   * @class Event
   * @brief Represents a single log event.
   *
   * This class holds details of a log event, including:
   * - Timestamp of when the event occurred.
   * - Log level (e.g., INFO, ERROR).
   * - Logger name that generated the event.
   * - Thread number and optional thread name.
   * - Formatted log message with limited storage.
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
    Event &operator=(const Event &) = delete;

    /**
     * @brief Constructs an event with formatted message data.
     * @tparam ThreadInfoType Type indicating thread info inclusion.
     * @tparam Format Type of the format string.
     * @tparam Args Types of formatting arguments.
     * @param name Logger name.
     * @param thread_info_type Specifies whether thread ID or name is stored.
     * @param level Severity level of the log event.
     * @param format Format string for the log message.
     * @param max_message_length Maximum message length allowed.
     * @param args Arguments for formatting the log message.
     */
    template <typename ThreadInfoType, typename Format, typename... Args>
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    Event(std::string_view name,
          ThreadInfoType thread_info_type,
          Level level,
          const Format &format,
          size_t max_message_length,
          const Args &...args)
        : timestamp_(std::chrono::system_clock::now()), level_(level) {
      // Capture thread information based on the logging configuration
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

      struct {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = char;
        using reference = value_type &;
        using pointer = value_type *;
        using difference_type = ptrdiff_t;

        value_type *pos;

        value_type &operator*() const {
          return *pos;
        }
        constexpr auto &operator++() {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          ++pos;
          return *this;
        }
        constexpr auto operator++(int) {
          auto origin = *this;
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          ++pos;
          return origin;
        }
      } it{message_data_};

      try {
        using OutputIt = decltype(it);
        message_size_ =
            ::fmt::vformat_to_n<OutputIt>(
                it,
                max_message_length,
                ::fmt::detail_exported::compile_string_to_view<char>(format),
                ::fmt::make_format_args(args...))
                .size;
      } catch (const std::exception &exception) {
        message_size_ = fmt::format_to_n(it,
                                         max_message_length,
                                         "Format error: {}; Format: {}",
                                         exception.what(),
                                         format)
                            .size;
        name = "Soralog";
        level_ = Level::ERROR;
      }

      // Ensure message does not exceed allowed length
      message_size_ = std::min(max_message_length, message_size_);
      name_size_ = std::min(name.size(), name_.size());
      std::copy_n(name.begin(), name_size_, name_.begin());
    }

    /**
     * @brief Gets the timestamp when the event occurred.
     * @return Timestamp of the event.
     */
    std::chrono::system_clock::time_point timestamp() const noexcept {
      return timestamp_;
    };

    /**
     * @brief Gets the thread number where the event was created.
     * @return Thread number.
     */
    size_t thread_number() const noexcept {
      return thread_number_;
    }

    /**
     * @brief Gets the thread name if available.
     * @return Thread name or empty string if not stored.
     */
    std::string_view thread_name() const noexcept {
      return {thread_name_.data(), thread_name_size_};
    }

    /**
     * @brief Gets the name of the logger that created this event.
     * @return Logger name.
     */
    std::string_view name() const noexcept {
      return {name_.data(), name_size_};
    }

    /**
     * @brief Gets the log level of the event.
     * @return Event's log level.
     */
    Level level() const noexcept {
      return level_;
    }

    /**
     * @brief Gets the formatted message of the event.
     * @return Log message.
     */
    std::string_view message() const noexcept {
      return {message_data_, message_size_};
    }

   private:
    std::chrono::system_clock::time_point timestamp_;  ///< Event timestamp.
    size_t thread_number_ = 0;          ///< Thread number of event origin.
    std::array<char, 16> thread_name_;  ///< Thread name.
    size_t thread_name_size_ = 0;       ///< Thread name size.
    std::array<char, 32> name_;         ///< Logger name.
    size_t name_size_;                  ///< Logger name size.
    Level level_ = Level::OFF;          ///< Log level of the event.

    // Pointer to message storage inside the object
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char *const message_data_ = reinterpret_cast<char *>(this) + sizeof(*this);
    size_t message_size_;  ///< Length of the log message.
  };

}  // namespace soralog
