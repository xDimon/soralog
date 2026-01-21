/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file logging_object.cpp
 * @brief Implementation of LoggingObject used in the full-feature example.
 *
 * This file contains the implementation of a simple component that owns a
 * per-instance logger obtained from LoggerFactory. The implementation focuses
 * on emitting log messages at different levels to demonstrate formatting,
 * filtering, and routing configured elsewhere.
 */

#include "logging_object.hpp"

// Obtain a logger instance for this object from the logger factory.
LoggingObject::LoggingObject(soralog::LoggerFactory &logger_factory)
    : log_(logger_factory.getLogger("ObjectTag", "example")) {}

// Emit log messages at all supported levels to demonstrate filtering.
void LoggingObject::method() const {
  // Lowest verbosity: usually disabled unless explicitly enabled.
  log_->trace("Example of trace log message");
  // Debug-level message with formatting arguments.
  log_->debug("There is a debug value in this line: {}", 0xDEADBEEF);
  // Verbose informational message (more detailed than INFO).
  log_->verbose("Let's gossip about something");
  // Regular informational message.
  log_->info("This is simple info message");
  // Warning message: something unexpected but recoverable.
  log_->warn("This is formatted message with level '{}'", "warning");
  // Error message: an operation failed, but the process continues.
  log_->error("This is message with level '{}' and number {}", "error", 777);
  // Critical message: severe problem, usually followed by a flush.
  log_->critical("This is example of critical situations");
}
