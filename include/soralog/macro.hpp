/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_MACROSES
#define SORALOG_MACROSES

#include <soralog/logger.hpp>

namespace soralog::macro {
  template <typename... Args>
  void proxy(const std::shared_ptr<soralog::Logger> &log, soralog::Level level,
             std::string_view fmt, Args &&... args) {
    if (log->level() >= level) {
      log->log(level, fmt, std::move(args)()...);
    }
  }
}  // namespace soralog::macro

#define WRAP_0(z)
#define WRAP_1(z, x) , [&] { return (x); }
#define WRAP_2(z, x, ...) , [&] { return (x); } WRAP_1(z, __VA_ARGS__)
#define WRAP_3(z, x, ...) , [&] { return (x); } WRAP_2(z, __VA_ARGS__)
#define WRAP_4(z, x, ...) , [&] { return (x); } WRAP_3(z, __VA_ARGS__)
#define WRAP_5(z, x, ...) , [&] { return (x); } WRAP_4(z, __VA_ARGS__)
#define WRAP_6(z, x, ...) , [&] { return (x); } WRAP_5(z, __VA_ARGS__)
#define WRAP_7(z, x, ...) , [&] { return (x); } WRAP_6(z, __VA_ARGS__)
#define WRAP_8(z, x, ...) , [&] { return (x); } WRAP_7(z, __VA_ARGS__)
#define WRAP_9(z, x, ...) , [&] { return (x); } WRAP_8(z, __VA_ARGS__)
#define WRAP_10(z, x, ...) , [&] { return (x); } WRAP_9(z, __VA_ARGS__)
#define WRAP_11(z, x, ...) , [&] { return (x); } WRAP_10(z, __VA_ARGS__)
#define WRAP_12(z, x, ...) , [&] { return (x); } WRAP_11(z, __VA_ARGS__)
#define WRAP_13(z, x, ...) , [&] { return (x); } WRAP_12(z, __VA_ARGS__)
#define WRAP_14(z, x, ...) , [&] { return (x); } WRAP_13(z, __VA_ARGS__)
#define WRAP_15(z, x, ...) , [&] { return (x); } WRAP_14(z, __VA_ARGS__)
#define WRAP_16(z, x, ...) , [&] { return (x); } WRAP_15(z, __VA_ARGS__)
#define WRAP_17(z, x, ...) , [&] { return (x); } WRAP_16(z, __VA_ARGS__)
#define WRAP_18(z, x, ...) , [&] { return (x); } WRAP_17(z, __VA_ARGS__)
#define WRAP_19(z, x, ...) , [&] { return (x); } WRAP_18(z, __VA_ARGS__)
#define WRAP_20(z, x, ...) , [&] { return (x); } WRAP_19(z, __VA_ARGS__)

#define WRAP_NARG(...) WRAP_NARG_(__VA_ARGS__, WRAP_RSEQ_N)
#define WRAP_NARG_(...) WRAP_ARG_N(__VA_ARGS__)
#define WRAP_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, \
                   _14, _15, _16, _17, _18, _19, _20, N, ...)                  \
  N
#define WRAP_RSEQ_N \
  20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define GET_WRAP_MACRO(x, y) GET_WRAP_MACRO_HELPER_1(x, y)
#define GET_WRAP_MACRO_HELPER_1(x, y) GET_WRAP_MACRO_HELPER_2(x, y)
#define GET_WRAP_MACRO_HELPER_2(x, y) x##y

#define WRAP_(N, x, ...) GET_WRAP_MACRO(WRAP_, N)(x, ##__VA_ARGS__)
#define WRAP(...) WRAP_(WRAP_NARG(__VA_ARGS__), ##__VA_ARGS__)

#define WRAP_ARGS(...) , ##__VA_ARGS__

#define SL_LOG(LOG, LVL, FMT, ...) \
  soralog::macro::proxy((LOG), (LVL), (FMT)WRAP(z WRAP_ARGS(__VA_ARGS__)))

#ifndef NDEBUG
#define SL_TRACE(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::TRACE, FMT, __VA_ARGS__)
#else
#define SL_TRACE(LOG, FMT, ...)
#endif

#define SL_DEBUG(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::DEBUG, FMT, __VA_ARGS__)

#define SL_VERBOSE(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::VERBOSE, FMT, __VA_ARGS__)

#define SL_INFO(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::INFO, FMT, __VA_ARGS__)

#define SL_WARN(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::WARN, FMT, __VA_ARGS__)

#define SL_ERROR(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::ERROR, FMT, __VA_ARGS__)

#define SL_CRITICAL(LOG, FMT, ...) \
  SL_LOG(LOG, soralog::Level::CRITICAL, FMT, __VA_ARGS__)

#endif  // SORALOG_MACROSES
