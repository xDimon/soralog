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
          - name: to_cout       # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stdout      # Output destination: 'stdout' (default) or 'stderr'
            level: trace        # Minimum log level handled by this sink
          - name: to_cerr       # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stderr      # Output destination: 'stdout' (default) or 'stderr'
            level: info         # Minimum log level handled by this sink
          - name: everywhere    # Unique identifier for this sink
            type: multisink     # 'multisink' type aggregates multiple sinks
            sinks:              # List of sinks where messages should be forwarded
              - to_cout
              - to_cerr

        groups:                 # Log groups define hierarchical loggers
          - name: main          # Root group handling logs
            sink: everywhere    # Default sink for this group
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
  auto logger = log_system.getLogger("Logger", "main");

  // Log message
  logger->trace("Example of trace log message");
  logger->debug("There is a debug value in this line: {}", 0xDEADBEEF);
  logger->verbose("Let's gossip about something");
  logger->info("This is simple info message");
  logger->warn("This is formatted message with level '{}'", "warning");
  logger->error("This is message with level '{}' and number {}", "error", 777);
  logger->critical("This is example of critical situations");
}
