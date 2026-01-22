/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: file logging using an embedded YAML configuration.
 *
 * This example uses ConfiguratorFromYAML with an in-memory YAML string.
 * The YAML defines a file sink and a fallback group that routes logs to it.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Embedded YAML configuration.
  // - Sink 'file' writes to /tmp/soralog_example.log
  // - Group 'main' routes logs to sink 'file' with level 'info'
  const auto yaml = std::string(R"(

sinks:
  - name: to_file
    type: file
    path: /tmp/soralog_example.log
    thread: name
    capacity: 2048
    buffer: 4194304
    latency: 1000
    at_fault: wait
    level: trace

groups:
  - name: main
    sink: to_file
    level: info
    is_fallback: true

)");

  // Create a YAML configurator that loads configuration from a string.
  auto cfg = std::make_shared<soralog::ConfiguratorFromYAML>(yaml);

  // Create the logging system with a configurator chain.
  soralog::LoggingSystem log_system(cfg);

  // Apply configuration waves (prepare, apply sinks, apply groups, cleanup).
  const auto r = log_system.configure();

  // Print diagnostics produced during configuration.
  if (not r.message.empty()) {
    std::cout << r.message;
  }
  if (r.has_error) {
    // Configuration errors are fatal for this example.
    return EXIT_FAILURE;
  }

  // Obtain a logger bound to the group defined in YAML.
  auto log = log_system.getLogger("FileExample", "main");

  // This debug is typically filtered by group 'main' level=info.
  log->debug("This debug is likely filtered out by group level=info");

  // These messages are written to /tmp/soralog_example.log.
  log->info("Started: mode={}, answer={}", "embedded_yaml", 42);
  log->warn("Disk logging example warning: code={}", 777);

  // Force flush before exiting.
  log->flush();

  return 0;
}
