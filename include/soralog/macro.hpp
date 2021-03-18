/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_MACROS
#define SORALOG_MACROS

#include <soralog/logger.hpp>

/**
 * SL_LOG
 * SL_TRACE
 * SL_DEBUG
 * SL_VERBOSE
 * SL_INFO
 * SL_WARN
 * SL_ERROR
 * SL_CRITICAL
 *
 * Macros is using to wrap logging argument to lambda to avoid calculation them
 * if their level not enough for logging
 */

namespace soralog::macro {
  template <typename... Args>
  inline void proxy(const std::shared_ptr<soralog::Logger> &log,
                    soralog::Level level, std::string_view fmt,
                    Args &&... args) {
    if (log->level() >= level) {
      log->log(level, fmt, std::move(args)()...);
    }
  }
}  // namespace soralog::macro

#define _SL_WRAP_0(z)
#define _SL_WRAP_1(z, x) , [&] { return (x); }
#define _SL_WRAP_2(z, x, ...) , [&] { return (x); } _SL_WRAP_1(z, __VA_ARGS__)
#define _SL_WRAP_3(z, x, ...) , [&] { return (x); } _SL_WRAP_2(z, __VA_ARGS__)
#define _SL_WRAP_4(z, x, ...) , [&] { return (x); } _SL_WRAP_3(z, __VA_ARGS__)
#define _SL_WRAP_5(z, x, ...) , [&] { return (x); } _SL_WRAP_4(z, __VA_ARGS__)
#define _SL_WRAP_6(z, x, ...) , [&] { return (x); } _SL_WRAP_5(z, __VA_ARGS__)
#define _SL_WRAP_7(z, x, ...) , [&] { return (x); } _SL_WRAP_6(z, __VA_ARGS__)
#define _SL_WRAP_8(z, x, ...) , [&] { return (x); } _SL_WRAP_7(z, __VA_ARGS__)
#define _SL_WRAP_9(z, x, ...) , [&] { return (x); } _SL_WRAP_8(z, __VA_ARGS__)
#define _SL_WRAP_10(z, x, ...) , [&] { return (x); } _SL_WRAP_9(z, __VA_ARGS__)
#define _SL_WRAP_11(z, x, ...) , [&] { return (x); } _SL_WRAP_10(z, __VA_ARGS__)
#define _SL_WRAP_12(z, x, ...) , [&] { return (x); } _SL_WRAP_11(z, __VA_ARGS__)
#define _SL_WRAP_13(z, x, ...) , [&] { return (x); } _SL_WRAP_12(z, __VA_ARGS__)
#define _SL_WRAP_14(z, x, ...) , [&] { return (x); } _SL_WRAP_13(z, __VA_ARGS__)
#define _SL_WRAP_15(z, x, ...) , [&] { return (x); } _SL_WRAP_14(z, __VA_ARGS__)
#define _SL_WRAP_16(z, x, ...) , [&] { return (x); } _SL_WRAP_15(z, __VA_ARGS__)
#define _SL_WRAP_17(z, x, ...) , [&] { return (x); } _SL_WRAP_16(z, __VA_ARGS__)
#define _SL_WRAP_18(z, x, ...) , [&] { return (x); } _SL_WRAP_17(z, __VA_ARGS__)
#define _SL_WRAP_19(z, x, ...) , [&] { return (x); } _SL_WRAP_18(z, __VA_ARGS__)
#define _SL_WRAP_20(z, x, ...) , [&] { return (x); } _SL_WRAP_19(z, __VA_ARGS__)

#define _SL_WRAP_NARG(...) _SL_WRAP_NARG_(__VA_ARGS__, _SL_WRAP_RSEQ_N)
#define _SL_WRAP_NARG_(...) _SL_WRAP_ARG_N(__VA_ARGS__)
#define _SL_WRAP_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                       _13, _14, _15, _16, _17, _18, _19, _20, N, ...)        \
  N
#define _SL_WRAP_RSEQ_N \
  20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define _SL_GET_WRAP_MACRO(x, y) _SL_GET_WRAP_MACRO_HELPER_1(x, y)
#define _SL_GET_WRAP_MACRO_HELPER_1(x, y) _SL_GET_WRAP_MACRO_HELPER_2(x, y)
#define _SL_GET_WRAP_MACRO_HELPER_2(x, y) x##y

#define _SL_WRAP_(N, x, ...) _SL_GET_WRAP_MACRO(_SL_WRAP_, N)(x, ##__VA_ARGS__)
#define _SL_WRAP(...) _SL_WRAP_(_SL_WRAP_NARG(__VA_ARGS__), ##__VA_ARGS__)

#define _SL_WRAP_ARGS(...) , ##__VA_ARGS__

#define SL_LOG(LOG, LVL, FMT, ...)    \
  soralog::macro::proxy((LOG), (LVL), \
                        (FMT)_SL_WRAP(z _SL_WRAP_ARGS(__VA_ARGS__)))

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

#endif  // SORALOG_MACROS
