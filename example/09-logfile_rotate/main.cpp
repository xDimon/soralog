/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: manual log rotation for a file sink.
 *
 * This example demonstrates a simple log rotation workflow:
 * 1) Configure a file sink that writes to a fixed path.
 * 2) Write a first message to the active log file.
 * 3) Rename the active log file to an "old" name.
 * 4) Call callRotateForAllSinks() to make file sinks reopen the path.
 * 5) Write a second message which goes to the newly opened file.
 *
 * Note: rotation is sink-specific. File sinks typically implement rotate by
 * closing the current file handle and reopening the configured path.
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/logger.hpp>

namespace {
  // Path as UTF-8, the encoding of YAML text
  std::string toUtf8(const std::filesystem::path &path) {
    auto str = path.u8string();
    return {str.begin(), str.end()};
  }
}  // namespace

int main() {
  // The log files go to the temporary directory of the system
  const auto dir = std::filesystem::temp_directory_path();
  const auto log_path = dir / "soralog_example.log";
  const auto old_log_path = dir / "soralog_example.old.log";

  // Ensure a clean start: remove any leftover files from previous runs.
  std::filesystem::remove(old_log_path);
  std::filesystem::remove(log_path);

  // Inline YAML configuration defining a single file sink and a main group.
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:                             # List of available logging sinks (outputs)
          - name: file                     # Unique identifier for this sink
            type: file                     # Log output to a file
            path: ')") + toUtf8(log_path) + R"(' # Path to the log file
            latency: 1000                  # Max delay before flushing logs to the file

        groups:                 # Log groups define hierarchical loggers
          - name: main          # Root group handling logs
            sink: file          # Default sink for this group
            level: trace        # Minimum log level for this group
            is_fallback: true   # This is the fallback group (only one allowed)
      )");

  // Initialize logging system
  soralog::LoggingSystem log_system(configurator);

  // Configure logging system
  auto r = log_system.configure();

  // Check the configuring result (useful for check-up config)
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    return EXIT_FAILURE;
  }

  // Obtain a logger
  auto logger = log_system.getLogger("Greeter", "main");

  // Write to the active log file.
  logger->info("First message");

  // Simulate rotation: move the current log to a backup name.
  std::filesystem::remove(old_log_path);
  std::filesystem::rename(log_path, old_log_path);

  // Ask all sinks to rotate (file sinks reopen the configured path).
  log_system.callRotateForAllSinks();

  // The worker thread reopens the file after it writes out the queued
  // events, so this message usually goes to the newly reopened file at the
  // original path, but may still go to the old one.
  logger->info("Second message");

  return 0;
}
