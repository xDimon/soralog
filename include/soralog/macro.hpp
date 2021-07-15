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
  template <typename Logger, typename... Args>
  inline void proxy(const std::shared_ptr<Logger> &log, soralog::Level level,
                    std::string_view fmt, Args &&... args) {
    if (log->level() >= level) {
      log->log(level, fmt, std::move(args)()...);
    }
  }
}  // namespace soralog::macro

#define _SL_WRAP_0(Z, x, ...)
#define _SL_WRAP_1(Z, x, ...) , [&] { return (x); }
#define _SL_WRAP_2(Z, x, ...) , [&] { return (x); } _SL_WRAP_1(Z, __VA_ARGS__)
#define _SL_WRAP_3(Z, x, ...) , [&] { return (x); } _SL_WRAP_2(Z, __VA_ARGS__)
#define _SL_WRAP_4(Z, x, ...) , [&] { return (x); } _SL_WRAP_3(Z, __VA_ARGS__)
#define _SL_WRAP_5(Z, x, ...) , [&] { return (x); } _SL_WRAP_4(Z, __VA_ARGS__)
#define _SL_WRAP_6(Z, x, ...) , [&] { return (x); } _SL_WRAP_5(Z, __VA_ARGS__)
#define _SL_WRAP_7(Z, x, ...) , [&] { return (x); } _SL_WRAP_6(Z, __VA_ARGS__)
#define _SL_WRAP_8(Z, x, ...) , [&] { return (x); } _SL_WRAP_7(Z, __VA_ARGS__)
#define _SL_WRAP_9(Z, x, ...) , [&] { return (x); } _SL_WRAP_8(Z, __VA_ARGS__)
#define _SL_WRAP_10(Z, x, ...) , [&] { return (x); } _SL_WRAP_9(Z, __VA_ARGS__)
#define _SL_WRAP_11(Z, x, ...) , [&] { return (x); } _SL_WRAP_10(Z, __VA_ARGS__)
#define _SL_WRAP_12(Z, x, ...) , [&] { return (x); } _SL_WRAP_11(Z, __VA_ARGS__)
#define _SL_WRAP_13(Z, x, ...) , [&] { return (x); } _SL_WRAP_12(Z, __VA_ARGS__)
#define _SL_WRAP_14(Z, x, ...) , [&] { return (x); } _SL_WRAP_13(Z, __VA_ARGS__)
#define _SL_WRAP_15(Z, x, ...) , [&] { return (x); } _SL_WRAP_14(Z, __VA_ARGS__)
#define _SL_WRAP_16(Z, x, ...) , [&] { return (x); } _SL_WRAP_15(Z, __VA_ARGS__)
#define _SL_WRAP_17(Z, x, ...) , [&] { return (x); } _SL_WRAP_16(Z, __VA_ARGS__)
#define _SL_WRAP_18(Z, x, ...) , [&] { return (x); } _SL_WRAP_17(Z, __VA_ARGS__)
#define _SL_WRAP_19(Z, x, ...) , [&] { return (x); } _SL_WRAP_18(Z, __VA_ARGS__)
#define _SL_WRAP_20(Z, x, ...) , [&] { return (x); } _SL_WRAP_19(Z, __VA_ARGS__)

#define _SL_WRAP_NARG(...) _SL_WRAP_NARG_(__VA_ARGS__, _SL_WRAP_RSEQ_N)
#define _SL_WRAP_NARG_(...) _SL_WRAP_ARG_N(__VA_ARGS__)
#define _SL_WRAP_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                       _13, _14, _15, _16, _17, _18, _19, _20, Z, N, ...)     \
  N
#define _SL_WRAP_RSEQ_N \
  20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define _SL_GET_WRAP_MACRO(x, y) _SL_GET_WRAP_MACRO_HELPER_1(x, y)
#define _SL_GET_WRAP_MACRO_HELPER_1(x, y) _SL_GET_WRAP_MACRO_HELPER_2(x, y)
#define _SL_GET_WRAP_MACRO_HELPER_2(x, y) x##y

#define _SL_WRAP_(N, x, ...) _SL_GET_WRAP_MACRO(_SL_WRAP_, N)(x, ##__VA_ARGS__)
#define _SL_WRAP(...) _SL_WRAP_(_SL_WRAP_NARG(__VA_ARGS__), ##__VA_ARGS__)

#define _SL_WRAP_ARGS(...) , ##__VA_ARGS__

#define _SL_LOG(LOG, LVL, FMT, ...)   \
  soralog::macro::proxy((LOG), (LVL), \
                        (FMT)_SL_WRAP(Z _SL_WRAP_ARGS(__VA_ARGS__)))

#define SL_LOG(LOG, LVL, FMT, ...) \
  _SL_LOG((LOG), (LVL), (FMT), ##__VA_ARGS__, Z)

/* You can use cmake options WITHOUT_TRACE_LOG_LEVEL and WITHOUT_DEBUG_LOG_LEVEL
   to remove (or not) debug and trace messages. See next cmake code for example:

option(WITHOUT_TRACE_LOG_LEVEL "Build without trace macro"        OFF)
option(WITHOUT_DEBUG_LOG_LEVEL "Build without debug macro"        OFF)
if (WITHOUT_TRACE_LOG_LEVEL AND NOT WITHOUT_DEBUG_LOG_LEVEL)
    set(WITHOUT_TRACE_LOG_LEVEL OFF)
    message(STATUS "Trace level have switched off, because bebug level is off")
endif() if (WITHOUT_TRACE_LOG_LEVEL)
    add_compile_definitions(WITHOUT_TRACE_LOG_LEVEL)
endif()
if (WITHOUT_DEBUG_LOG_LEVEL)
    add_compile_definitions(WITHOUT_DEBUG_LOG_LEVEL)
endif()

*/

#if defined(WITHOUT_DEBUG_LOG_LEVEL) and not defined(WITHOUT_TRACE_LOG_LEVEL)
#warning "Trace log level have switched off, because bebug log level is off"
#undef WITHOUT_DEBUG_LOG_LEVEL
#endif

#ifndef WITHOUT_TRACE_LOG_LEVEL
#define SL_TRACE(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::TRACE, (FMT), ##__VA_ARGS__, Z)
#else
#define SL_TRACE(LOG, FMT, ...)
#endif

#ifndef WITHOUT_DEBUG_LOG_LEVEL
#define SL_DEBUG(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::DEBUG, (FMT), ##__VA_ARGS__, Z)
#else
#define SL_DEBUG(LOG, FMT, ...)
#endif

#define SL_VERBOSE(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::VERBOSE, (FMT), ##__VA_ARGS__, Z)

#define SL_INFO(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::INFO, (FMT), ##__VA_ARGS__, Z)

#define SL_WARN(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::WARN, (FMT), ##__VA_ARGS__, Z)

#define SL_ERROR(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::ERROR, (FMT), ##__VA_ARGS__, Z)

#define SL_CRITICAL(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::CRITICAL, (FMT), ##__VA_ARGS__, Z)

#endif  // SORALOG_MACROS
