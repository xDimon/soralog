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
 */

#define _SL_LOG_IF_LEVEL(LOG, LVL, FMT, ...)                 \
  do {                                                       \
    auto &&_sl_log_log = (LOG);                              \
    soralog::Level _sl_log_level = (LVL);                    \
    if (_sl_log_log->level() >= _sl_log_level) {             \
      _sl_log_log->log(_sl_log_level, (FMT), ##__VA_ARGS__); \
    }                                                        \
  } while (false)

#define _SL_LOG(LOG, LVL, FMT, ...) \
  _SL_LOG_IF_LEVEL((LOG), (LVL), FMT_STRING(FMT), ##__VA_ARGS__)

#define SL_LOG(LOG, LVL, FMT, ...) _SL_LOG((LOG), (LVL), (FMT), ##__VA_ARGS__)

#define _SL_LOG_DF(LOG, LVL, FMT, ...) \
  _SL_LOG_IF_LEVEL((LOG), (LVL), (FMT), ##__VA_ARGS__)

#define SL_LOG_DF(LOG, LVL, FMT, ...) \
  _SL_LOG_DF((LOG), (LVL), (FMT), ##__VA_ARGS__)

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

// Macros for string-literal format
// Correctness of their format will be validated in compile-time

#ifndef WITHOUT_TRACE_LOG_LEVEL
#define SL_TRACE(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::TRACE, (FMT), ##__VA_ARGS__)
#else
#define SL_TRACE(LOG, FMT, ...)
#endif

#ifndef WITHOUT_DEBUG_LOG_LEVEL
#define SL_DEBUG(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::DEBUG, (FMT), ##__VA_ARGS__)
#else
#define SL_DEBUG(LOG, FMT, ...)
#endif

#define SL_VERBOSE(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::VERBOSE, (FMT), ##__VA_ARGS__)

#define SL_INFO(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::INFO, (FMT), ##__VA_ARGS__)

#define SL_WARN(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::WARN, (FMT), ##__VA_ARGS__)

#define SL_ERROR(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::ERROR, (FMT), ##__VA_ARGS__)

#define SL_CRITICAL(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::CRITICAL, (FMT), ##__VA_ARGS__)

// Macros for others () format, i.e. string, string-view, null-terminated-string
// They are differs by suffix '_DF' (dynamic format).
// Correctness of format will be validated in runtime

#ifndef WITHOUT_TRACE_LOG_LEVEL
#define SL_TRACE_DF(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::TRACE, (FMT), ##__VA_ARGS__)
#else
#define SL_TRACE_DF(LOG, FMT, ...)
#endif

#ifndef WITHOUT_DEBUG_LOG_LEVEL
#define SL_DEBUG_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::DEBUG, (FMT), ##__VA_ARGS__)
#else
#define SL_DEBUG_DF(LOG, FMT, ...)
#endif

#define SL_VERBOSE_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::VERBOSE, (FMT), ##__VA_ARGS__)

#define SL_INFO_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::INFO, (FMT), ##__VA_ARGS__)

#define SL_WARN_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::WARN, (FMT), ##__VA_ARGS__)

#define SL_ERROR_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::ERROR, (FMT), ##__VA_ARGS__)

#define SL_CRITICAL_DF(LOG, FMT, ...) \
  _SL_LOG_DF((LOG), soralog::Level::CRITICAL, (FMT), ##__VA_ARGS__)

#endif  // SORALOG_MACROS
