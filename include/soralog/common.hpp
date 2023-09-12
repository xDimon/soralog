/**
 * Copyright Quadrivium Co. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>

// clang-format off
#if not defined(likely_if) or not defined(unlikely_if)
  #if __cplusplus > 201703L
    #define likely_if(x) [[likely]] if (x)
    #define unlikely_if(x) [[unlikely]] if (x)
  #elif defined( __has_builtin)
    #if __has_builtin(__builtin_expect)
      #define likely_if(x) if (__builtin_expect((x), 1))
      #define unlikely_if(x) if (__builtin_expect((x), 0))
    #else
      #define likely_if(x) if (x)
      #define unlikely_if(x) if (x)
    #endif
  #else
    #define likely_if(x) if (x)
    #define unlikely_if(x) if (x)
  #endif
#endif
// clang-format on

#include <fmt/format.h>

namespace soralog::fmt {

  using namespace ::fmt;

  template <typename Format, typename... Args>
  inline auto format(Format format, Args &&...args) {
    if constexpr (std::is_constructible_v<::fmt::format_string<Args...>>) {
      return ::fmt::format(format, std::forward<Args>(args)...);
    } else {
      return ::fmt::vformat(
          format, ::fmt::make_format_args(std::forward<Args>(args)...));
    }
  }

  template <typename OutputIt, typename Format, typename... Args>
  auto format_to_n(OutputIt out, size_t size, const Format &format,
                   Args &&...args) {
    if constexpr (sizeof...(args) == 0) {
      struct R {
        OutputIt out;
        size_t size;
      };
      auto f = ::fmt::detail_exported::compile_string_to_view<char>(format);
      auto n = std::min(size, std::size(f));
      return R{std::copy_n(std::begin(f), n, out), n};
    } else {
      return ::fmt::vformat_to_n(
          out, size,
          ::fmt::detail_exported::compile_string_to_view<char>(format),
          ::fmt::make_format_args(args...));
    }
  }

}  // namespace soralog::fmt
