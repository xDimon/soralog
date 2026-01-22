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

int main() {
  // Ensure a clean start: remove any leftover files from previous runs.
  std::filesystem::remove("/tmp/solalog_example.old.log");
  std::filesystem::remove("/tmp/solalog_example.log");

  // Inline YAML configuration defining a single file sink and a main group.
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:                             # List of available logging sinks (outputs)
          - name: file                     # Unique identifier for this sink
            type: file                     # Log output to a file
            path: /tmp/solalog_example.log # Path to the log file
            latency: 1000                  # Max delay before flushing logs to the file

        groups:                 # Log groups define hierarchical loggers
          - name: main          # Root group handling logs
            sink: file          # Default sink for this group
            level: trace        # Minimum log level for this group
            is_fallback: true   # This is the fallback group (only one allowed)
      )"));

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

  // Write to the active log file (/tmp/solalog_example.log).
  logger->info("First message");

  // Simulate rotation: move the current log to a backup name.
  std::filesystem::remove("/tmp/solalog_example.old.log");
  std::filesystem::rename("/tmp/solalog_example.log", "/tmp/solalog_example.old.log");

  // Ask all sinks to rotate (file sinks reopen the configured path).
  log_system.callRotateForAllSinks();

  // This message goes to the newly reopened /tmp/solalog_example.log.
  logger->info("Second message");

  return 0;
}
