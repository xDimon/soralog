/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging_object.hpp"

LoggingObject::LoggingObject(soralog::LoggerFactory &logger_factory)
    : log_(logger_factory.getLogger("ObjectTag", "example")) {}

void LoggingObject::method() const {
  log_->trace("Example of trace log message");
  log_->debug("There is a debug value in this line: {}", 0xDEADBEEF);
  log_->verbose("Let's gossip about something");
  log_->info("This is simple info message");
  log_->warn("This is formatted message with level '{}'", "warning");
  log_->error("This is message with level '{}' and number {}", "error", 777);
  log_->critical("This is example of critical situations");
}
