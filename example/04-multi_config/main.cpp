/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: embedded YAML configuration and configurator chaining.
 *
 * This example demonstrates how multiple configurators are applied in order.
 * The logging system runs configuration in four waves:
 * 1) prepare()  - each configurator loads/validates its config
 * 2) applySinks()- sinks are created/overridden in configurator order
 * 3) applyGroups()- groups are created/overridden in configurator order
 * 4) cleanup()  - configurators release temporary state
 *
 * If two configurators define the same sink/group, the later configurator in
 * the chain overrides earlier settings.
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // A secondary configurator (e.g. from a 3rd-party module or a preset).
  // This configurator overrides the level of the already-declared group.
  auto external_configurator =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
        groups:
          - name: external
            level: debug
      )"));

  // The main configurator defines sinks and the base group tree.
  // It declares a console sink and makes group 'main' the fallback group.
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

  // Create a logging system with a configurator chain (left-to-right order).
  soralog::LoggingSystem log_system(configurator, external_configurator);
  // Order matters:
  // - First configurator creates sink 'console' and groups 'main' -> 'external'
  // - Second configurator then overrides group 'external' level to 'debug'.

  // Apply configuration waves across all configurators in the given order.
  auto r = log_system.configure();

  // Print diagnostics (warnings/errors) produced during configuration.
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    // Configuration errors are fatal for this example.
    return EXIT_FAILURE;
  }

  // Create a logger bound to group 'external' (declared in YAML).
  auto logger = log_system.getLogger("Greeter", "external");

  // Log messages: debug is visible only after the override to 'debug'.
  logger->info("Info is enabled by 'main' (level: info)");
  logger->debug("Debug is enabled only after the override (level: debug)");
  logger->trace("Trace is typically filtered out by default");
}
