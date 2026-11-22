/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// <windows.h> defines macros ERROR and IGNORE, named like values used here:
// hide them for this header and restore after, whatever the include order
#pragma push_macro("ERROR")
#pragma push_macro("IGNORE")
#undef ERROR
#undef IGNORE

namespace soralog {

  /**
   * @enum Level
   * @brief Defines the level of detail for logging or events.
   */
  enum class Level : uint8_t {
    OFF = 0,   ///< No logging (for disabling logger)
    FATAL,     ///< Logs only fatal messages
    CRITICAL,  ///< Logs only critical messages
    ERROR,     ///< Logs error messages
    WARN,      ///< Logs warning messages
    INFO,      ///< Logs important informational messages
    VERBOSE,   ///< Logs all information messages
    DEBUG,     ///< Logs debugging messages
    TRACE,     ///< Logs trace events
    IGNORE,    ///< No logging (for specific messages)
  };

  namespace detail {
    /**
     * @brief Mapping of log levels to their string representations.
     */
    constexpr std::array<const char *, static_cast<uint8_t>(Level::IGNORE) + 1>
        level_to_str_map = [] {
          std::array<const char *, static_cast<uint8_t>(Level::IGNORE) + 1> r{};
          r[static_cast<uint8_t>(Level::OFF)] = "?Off";
          r[static_cast<uint8_t>(Level::FATAL)] = "Fatal";
          r[static_cast<uint8_t>(Level::CRITICAL)] = "Critical";
          r[static_cast<uint8_t>(Level::ERROR)] = "Error";
          r[static_cast<uint8_t>(Level::WARN)] = "Warning";
          r[static_cast<uint8_t>(Level::INFO)] = "Info";
          r[static_cast<uint8_t>(Level::VERBOSE)] = "Verbose";
          r[static_cast<uint8_t>(Level::DEBUG)] = "Debug";
          r[static_cast<uint8_t>(Level::TRACE)] = "Trace";
          r[static_cast<uint8_t>(Level::IGNORE)] = "?Ignore";
          return r;
        }();

    /// Level::ERROR for macros, which expand in user code, where ERROR may be
    /// a macro of <windows.h>
    inline constexpr Level level_error = Level::ERROR;
  }  // namespace detail

  /**
   * @brief Converts a logging level to its representative character.
   * @param level The logging level.
   * @return The first character of the level's string representation.
   */
  constexpr char levelToChar(Level level) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return detail::level_to_str_map[static_cast<uint8_t>(level)][0];
  }

  /**
   * @brief Converts a logging level to its string representation.
   * @param level The logging level.
   * @return A C-string representing the given logging level.
   */
  constexpr const char *levelToStr(Level level) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return detail::level_to_str_map[static_cast<uint8_t>(level)];
  }

}  // namespace soralog

#pragma pop_macro("IGNORE")
#pragma pop_macro("ERROR")
