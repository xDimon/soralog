/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LEVEL
#define SORALOG_LEVEL

#include <array>
#include <cstddef>
#include <cstdint>

namespace soralog {

  /**
   * Level detalization of logging or event
   */
  enum class Level : uint8_t {
    OFF = 0,   /// No log
    CRITICAL,  /// Log only critical
    ERROR,     /// Error
    WARN,      /// Warning
    INFO,      /// Important information
    VERBOSE,   /// All information
    DEBUG,     /// Message for debug
    TRACE,     /// Trace event
  };

  namespace detail {
    constexpr std::array<const char *, static_cast<uint8_t>(Level::TRACE) + 1>
        level_to_str_map = [] {
          std::array<const char *, static_cast<uint8_t>(Level::TRACE) + 1> r{};
          r[static_cast<uint8_t>(Level::OFF)] = "?Off";
          r[static_cast<uint8_t>(Level::CRITICAL)] = "Critical";
          r[static_cast<uint8_t>(Level::ERROR)] = "Error";
          r[static_cast<uint8_t>(Level::WARN)] = "Warning";
          r[static_cast<uint8_t>(Level::INFO)] = "Info";
          r[static_cast<uint8_t>(Level::VERBOSE)] = "Verbose";
          r[static_cast<uint8_t>(Level::DEBUG)] = "Debug";
          r[static_cast<uint8_t>(Level::TRACE)] = "Trace";
          return r;
        }();
  }

  /**
   * @returns symbol in according with {@param level}
   */
  constexpr char levelToChar(Level level) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return detail::level_to_str_map[static_cast<uint8_t>(level)][0];
  }

  /**
   * @returns C-string in according with {@param level}
   */
  constexpr const char *levelToStr(Level level) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return detail::level_to_str_map[static_cast<uint8_t>(level)];
  }

}  // namespace soralog

#endif  // SORALOG_LEVEL
