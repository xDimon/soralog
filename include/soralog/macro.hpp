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
 * @brief Logging macros for different log levels.
 *
 * This file defines a set of convenience macros for logging messages with
 * automatic log level checking.
 *
 * The macros forward the provided format and arguments to the underlying
 * logger implementation. The actual handling of format types
 * (string literals, std::string, std::string_view, C-strings) and any
 * format validation is performed by the logger, not by the macros themselves.
 *
 * Optionally, trace and debug logging can be disabled at compile time using:
 * - `WITHOUT_TRACE_LOG_LEVEL`
 * - `WITHOUT_DEBUG_LOG_LEVEL`
 */

/**
 * @brief Base macro that logs a message if the specified log level is enabled.
 *
 * This macro evaluates the logger expression once, compares the current
 * log level with the requested one, and forwards the call to
 * `Logger::log(...)` if logging is enabled.
 */
#define _SL_LOG_IF_LEVEL(LOG, LVL, FMT, ...)                 \
  ({                                                         \
    auto &&_sl_log_log = (LOG);                              \
    soralog::Level _sl_log_level = (LVL);                    \
    if (_sl_log_log->level() >= _sl_log_level) {             \
      _sl_log_log->log(_sl_log_level, (FMT), ##__VA_ARGS__); \
    }                                                        \
  })

/**
 * @brief Internal logging macro.
 *
 * This macro is a thin wrapper around `_SL_LOG_IF_LEVEL` and does not
 * impose any restrictions on the format argument type.
 */
#define _SL_LOG(LOG, LVL, FMT, ...) \
  _SL_LOG_IF_LEVEL((LOG), (LVL), (FMT), ##__VA_ARGS__)

/**
 * @brief Public macro for generic logging with an explicit log level.
 */
#define SL_LOG(LOG, LVL, FMT, ...) _SL_LOG((LOG), (LVL), (FMT), ##__VA_ARGS__)

/**
 * @section Conditional Compilation for Debug and Trace Logs
 *
 * Trace and debug logging can be removed at compile time by defining:
 * - `WITHOUT_TRACE_LOG_LEVEL`
 * - `WITHOUT_DEBUG_LOG_LEVEL`
 *
 * If debug logging is disabled, trace logging is disabled automatically.
 */

#if defined(WITHOUT_DEBUG_LOG_LEVEL) and not defined(WITHOUT_TRACE_LOG_LEVEL)
#warning "Trace log level has been disabled because debug log level is off"
#undef WITHOUT_DEBUG_LOG_LEVEL
#endif

/**
 * @section Logging Macros by Level
 *
 * The following macros log messages at fixed log levels.
 * They perform a runtime log level check and forward the call to the logger.
 */

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

#define SL_FATAL(LOG, FMT, ...) \
  _SL_LOG((LOG), soralog::Level::FATAL, (FMT), ##__VA_ARGS__);
