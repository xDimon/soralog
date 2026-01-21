/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: using a multisink to log to multiple outputs.
 *
 * This example demonstrates how to configure a `multisink` that forwards each
 * log event to several underlying sinks. Here, we route the same message to
 * both stdout and stderr by aggregating two console sinks.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Inline YAML configuration defining two console sinks and a multisink.
  // The multisink forwards each event to both underlying sinks.
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:                  # List of available logging sinks (outputs)
          - name: to_cout       # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stdout      # Output destination: 'stdout' (default) or 'stderr'
          - name: to_cerr       # Unique identifier for this sink
            type: console       # Sink type: 'console' means output to stdout or stderr
            stream: stderr      # Output destination: 'stdout' (default) or 'stderr'
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

  // Create the logging system using the YAML configurator.
  soralog::LoggingSystem log_system(configurator);

  // Apply configuration (prepare, apply sinks, apply groups, cleanup).
  auto r = log_system.configure();

  // Print configuration diagnostics (warnings or errors).
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    // Configuration errors are fatal for this example.
    return EXIT_FAILURE;
  }

  // Obtain a logger bound to group 'main' which uses the multisink.
  auto logger = log_system.getLogger("Logger", "main");

  // This message is duplicated: it is written to stdout and stderr.
  logger->info("Hello, stdout'n'stderr!");
  logger->warn("This warning is also duplicated by the multisink");
}
