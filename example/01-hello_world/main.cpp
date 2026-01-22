/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: minimal "Hello, world" using an inline YAML configuration.
 *
 * This example demonstrates the typical flow:
 * 1) Build a configurator (here: YAML provided as an in-memory string).
 * 2) Create a LoggingSystem with one (or more) configurators.
 * 3) Call configure() and handle diagnostics.
 * 4) Obtain a Logger and emit log messages.
 */

#include <iostream>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/logger.hpp>

int main() {
  // Create a configurator from an inline YAML document (in-memory config).
  // The YAML defines sinks (outputs) and groups (routing + levels).
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:                  # List of available logging sinks (outputs)
          - name: console       # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            color: true         # Enables colored output using ANSI escape codes

        groups:                 # Log groups define hierarchical loggers
          - name: main          # Root group handling logs
            sink: console       # Default sink for this group
            level: trace        # Minimum log level for this group
            is_fallback: true   # This is the fallback group (only one allowed)
      )"));

  // Create the logging system. Multiple configurators may be chained.
  soralog::LoggingSystem log_system(configurator);

  // Apply configuration in waves (prepare, sinks, groups, cleanup).
  auto r = log_system.configure();

  // Print diagnostics produced during configuration.
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  // Abort the program if configuration reported a critical error.
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  // Obtain a logger bound to the specified group (routing and level).
  auto logger = log_system.getLogger("Greeter", "main");

  // Emit a log message using fmt-style formatting (see logger API).
  // The message goes to the group's sink (console) if level allows it.
  logger->info("Hello, world!");
}
