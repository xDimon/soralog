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



 // Use configurator with yaml-file
 auto configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(
          std::filesystem::path("../../../example/03-with_config_file/logger.yml"));

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
