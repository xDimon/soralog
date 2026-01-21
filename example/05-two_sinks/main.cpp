/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: using two sinks (stdout and stderr).
 *
 * This example demonstrates how to configure multiple sinks and route log
 * output to different destinations using groups:
 * - Group `one` writes to stdout
 * - Group `two` writes to stderr
 *
 * Each logger is bound to a group, and the group defines which sink is used.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Inline YAML configuration defining two console sinks and two groups.
  // Each sink is bound to a different output stream.
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

  // Create loggers bound to different groups (and thus different sinks).
  auto logger1 = log_system.getLogger("Logger1", "one");
  // Uses group 'one' -> sink 'cout' (stdout)
  auto logger2 = log_system.getLogger("Logger2", "two");
  // Uses group 'two' -> sink 'cerr' (stderr)

  // Each message is routed to the sink defined by its group.
  logger1->info("Hello, stdout!");
  logger2->info("Hello, stderr!");
}
