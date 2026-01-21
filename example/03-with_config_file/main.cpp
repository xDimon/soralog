/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: configuring soralog using an external YAML file.
 *
 * This example demonstrates how to configure the logging system from a YAML
 * configuration file:
 * - load configuration via ConfiguratorFromYAML
 * - create LoggingSystem with a configurator chain
 * - apply configuration and handle diagnostics
 * - obtain a logger bound to a group defined in YAML
 */

#include <filesystem>
#include <iostream>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {

  // Create a YAML configurator that loads configuration from a file.
  // Path to the YAML file describing sinks and groups.
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(
          std::filesystem::path("../../../example/03-with_config_file/logger.yml"));

  // Create the logging system with the YAML configurator.
  soralog::LoggingSystem log_system(configurator);

  // Apply the configuration (prepare, apply sinks, apply groups, cleanup).
  auto r = log_system.configure();

  // Print configuration diagnostics and abort on critical errors.
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  // Obtain a logger bound to the group defined in the YAML configuration.
  auto logger = log_system.getLogger("Greeter", "main");

  // Emit a log message routed according to the YAML-defined group and sink.
  logger->info("Hello, world!");
}
