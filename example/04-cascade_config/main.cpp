/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>

int main() {
  // Some underlying configurator (i.e. 3rd-party module, default preset, etc.)
  auto external_configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        groups:
          - name: external
            level: debug
      )"));

  // Use configurator with inline yaml content
  auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        sinks:
          - name: console
            type: console
            color: true
        groups:
          - name: main
            sink: console
            level: info
            is_fallback: true
            children:
              - name: external
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
  auto logger = log_system.getLogger("Greeter", "external");

  // Log message
  logger->debug("Hello, world!");
}
