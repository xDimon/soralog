/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Use configurator with inline yaml content
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:                  # List of available logging sinks (outputs)
          - name: cout          # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stdout      # Output destination: 'stdout' (default) or 'stderr'
          - name: cerr          # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stderr      # Output destination: 'stdout' (default) or 'stderr'
        groups:                 # Log groups define hierarchical loggers
          - name: one           # Name of group
            sink: cout          # Default sink for this group
            level: trace        # Minimum log level for this group
          - name: two           # Name of group
            sink: cerr          # Default sink for this group
            level: trace        # Minimum log level for this group
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
  auto logger1 = log_system.getLogger("Logger1", "one");
  auto logger2 = log_system.getLogger("Logger2", "two");

  // Log message
  logger1->info("Hello, stdout!");
  logger2->info("Hello, stderr!");
}
