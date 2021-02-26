/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging_object.hpp"

LoggingObject::LoggingObject(soralog::LoggerFactory &logger_factory)
    : log_(logger_factory.get("ObjectTag", "console", soralog::Level::TRACE)) {}

void LoggingObject::method() const {
  auto start_time = std::chrono::system_clock::now();

  log_->trace("Example of trace log message");
  log_->debug("There is a debug value in this line: {}", 0xDEADBEEF);
  log_->verbose("Let's gossip about something");
  log_->info("This is simple info message");
  log_->warn("This is formatted message with level '{}'", "warning");
  log_->error("This is message with level '{}' and number {}", "error", 777);
  log_->critical("This is example of critical situations");

  const bool benchmark = false;

  if constexpr (benchmark) {
    static size_t count = 7;
    size_t all = 100'000'000;
    while (count < all) {
      ++count;
      log_->trace("Trace message #{}", count);
    }

    auto end_time = std::chrono::system_clock::now();

    auto time_spent = end_time - start_time;
    auto st =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(time_spent)
                .count())
        / 1'000'000;

    log_->info("Spent {} sec", st);
    log_->info("Speed {} Mmps", (std::round(all / st / 1'000) / 1'000));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Spent " << st << " sec" << std::endl;
    std::cout << "Speed " << (std::round(all / st / 1'000) / 1'000) << " Mmps"
              << std::endl;
  }
}
