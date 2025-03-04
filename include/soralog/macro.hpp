/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/logger.hpp>

/**
 * @file log_macros.hpp
 * @brief Provides logging macros for different log levels.
 *
 * These macros simplify logging calls by automatically checking log levels
 * and formatting messages. Two sets of macros are available:
 * - Standard format (compile-time validation).
 * - Dynamic format (_DF suffix, runtime validation).
 *
 * Optionally, trace and debug logging can be disabled at compile time using:
 * - `WITHOUT_TRACE_LOG_LEVEL`
 * - `WITHOUT_DEBUG_LOG_LEVEL`
 */

// Base macro that logs a message if the specified log level is enabled.
#define _SL_LOG_IF_LEVEL(LOG, LVL, FMT, ...)                 \
  ({                                                         \
    auto &&_sl_log_log = (LOG);                              \
    soralog::Level _sl_log_level = (LVL);                    \
    if (_sl_log_log->level() >= _sl_log_level) {             \
      _sl_log_log->log(_sl_log_level, (FMT), ##__VA_ARGS__); \
    }                                                        \
  })

// Standard logging macro.
#define _SL_LOG(LOG, LVL, FMT, ...) \
  _SL_LOG_IF_LEVEL((LOG), (LVL), (FMT), ##__VA_ARGS__)

// Public macro for general logging.
#define SL_LOG(LOG, LVL, FMT, ...) _SL_LOG((LOG), (LVL), (FMT), ##__VA_ARGS__)

// Dynamic format version (runtime format validation).
#define _SL_LOG_DF(LOG, LVL, FMT, ...) \
  _SL_LOG_IF_LEVEL((LOG), (LVL), (FMT), ##__VA_ARGS__)

#define SL_LOG_DF(LOG, LVL, FMT, ...) \
  _SL_LOG_DF((LOG), (LVL), (FMT), ##__VA_ARGS__)

/**
 * @section Conditional Compilation for Debug and Trace Logs
 *
 * To disable trace/debug logs at compile time, define:
 * - `WITHOUT_TRACE_LOG_LEVEL`
 * - `WITHOUT_DEBUG_LOG_LEVEL`
 *
 * If trace logs are disabled, debug logs will be disabled automatically.
 */

#if defined(WITHOUT_DEBUG_LOG_LEVEL) and not defined(WITHOUT_TRACE_LOG_LEVEL)
#warning "Trace log level has been disabled because debug log level is off"
#undef WITHOUT_DEBUG_LOG_LEVEL
#endif

// Macros for standard logging levels (compile-time format validation).

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
  _SL_LOG_DF((LOG), soralog::Level::TRACE, (FMT), ##__VA_ARGS__)
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