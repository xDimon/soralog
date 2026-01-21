/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: multisink with different per-sink log levels.
 *
 * A `multisink` forwards each log event to multiple underlying sinks.
 * Each underlying sink still applies its own level filter.
 *
 * In this example:
 * - `to_cout` accepts everything from TRACE and above (stdout).
 * - `to_cerr` accepts only INFO and above (stderr).
 *
 * As a result, TRACE/DEBUG/VERBOSE messages appear only on stdout, while
 * INFO/WARN/ERROR/CRITICAL appear on both stdout and stderr.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Inline YAML configuration defining a multisink with per-sink levels.
  // Note: the multisink forwards to both sinks, but each sink filters by level.
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

  // Observe routing: low levels go only to stdout; INFO+ go to both sinks.
  // Below INFO: expected only on stdout (to_cout).
  logger->trace("Trace: stdout only (to_cout level=trace)");
  logger->debug("Debug: stdout only, value={}", 0xDEADBEEF);
  logger->verbose("Verbose: stdout only");

  // INFO and above: expected on both stdout and stderr.
  logger->info("Info: stdout + stderr (to_cerr level=info)");
  logger->warn("Warn: duplicated by multisink");
  logger->error("Error: duplicated, code={}", 777);
  logger->critical("Critical: duplicated (and will flush)");

  return 0;
}
