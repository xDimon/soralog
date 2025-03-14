/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/logger.hpp>

int main() {
  // Use configurator with inline yaml content
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

  // Initialize logging system
  soralog::LoggingSystem log_system(configurator);

  // Configure logging system
  auto r = log_system.configure();

  // Check the configuring result (useful for check-up config)
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  // Obtain a logger
  auto logger = log_system.getLogger("Greeter", "main");

  // Log message
  logger->info("Hello, world!");
}
