/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>
#include <chrono>
#include <ctime>
#include <string>
#include <string_view>

namespace soralog::detail {

  using namespace std::chrono_literals;

  inline std::tm localtime(std::time_t t) {
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
  }

  // string literal
  template <typename Char, std::size_t N>
  constexpr fmt::basic_string_view<Char> compile_string_to_view(
      const Char (&s)[N]) {
    return {s, N > 0 ? N - 1 : 0};
  }

  // std::string_view
  template <typename Char>
  constexpr fmt::basic_string_view<Char> compile_string_to_view(
      std::basic_string_view<Char> s) {
    return {s.data(), s.size()};
  }

  // const Char* / Char* (but NOT arrays)
  template <typename Ptr>
    requires(std::is_pointer_v<std::remove_reference_t<Ptr>>
             && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<
                                   std::remove_reference_t<Ptr>>>,
                               char>)
  inline fmt::basic_string_view<char> compile_string_to_view(Ptr s) {
    const char *p = s;
    return p ? fmt::string_view(p, std::char_traits<char>::length(p))
             : fmt::string_view();
  }

  // std::string / std::basic_string
  template <typename Char, typename Traits, typename Alloc>
  inline fmt::basic_string_view<Char> compile_string_to_view(
      const std::basic_string<Char, Traits, Alloc> &s) {
    return {s.data(), s.size()};
  }

}  // namespace soralog::detail
