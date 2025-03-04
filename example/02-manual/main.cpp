/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <soralog/group.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/impl/fallback_configurator.hpp>
#include <soralog/impl/sink_to_console.hpp>
#include <soralog/logger.hpp>

int main() {
  // Use configurator with inline yaml content
  auto log_sys_cfg = std::make_shared<soralog::FallbackConfigurator>();

  // Initialize logging system
  soralog::LoggingSystem log_system(log_sys_cfg);

  // Manual creation of a sink
  log_system.makeSink<soralog::SinkToConsole>(
      "sinkName",
      soralog::Level::INFO,
      soralog::SinkToConsole::Stream::STDOUT,
      true);

  // Manual creation of a group
  auto group1 =
      log_system.makeGroup("group1", {}, "sinkName", soralog::Level::INFO);

  // Assign a group as fallback
  log_system.setFallbackGroup("group1");

  // Manual creation of a group and setup
  log_system.makeGroup("group2", "group1", {}, {});

  // Get a group by name
  auto group2 = log_system.getGroup("group2");

  // Manual post-setup of the group directly
  group2->setLevel(soralog::Level::INFO);
  group2->setParentGroup("group1");
  group2->setSink("sink1");
  group2->setLevel(soralog::Level::WARN);

  // Inherit some properties from another group
  group2->setLevelFromGroup("group1");
  group2->setSinkFromGroup("group1");

  // Reset some properties and inherent them from the parent group
  group2->resetLevel();
  group2->resetSink();

  // Forget a parent group and become independent
  group2->unsetParentGroup();

  // Manual post-setup of the group over logging system
  log_system.setParentOfGroup("group2", "group1");
  log_system.setLevelOfGroup("group2", soralog::Level::INFO);
  log_system.setSinkOfGroup("group2", "sink1");

  // Reset some properties and inherent them from the parent group
  log_system.resetLevelOfGroup("group2");
  log_system.resetSinkOfGroup("group2");

  // Create logger and inherent setting from a specified group
  auto logger1 = log_system.getLogger("logger1", "group1");

  // Create logger and set properties explicitly
  auto logger2 = log_system.getLogger("logger2", "group1", "sink1");
  auto logger3 =
      log_system.getLogger("logger3", "group2", soralog::Level::TRACE);
  auto logger4 =
      log_system.getLogger("logger4", "group1", "sink1", soralog::Level::INFO);

  // Manual setup of the logger
  log_system.setGroupOfLogger("logger1", "group2");
  log_system.setLevelOfLogger("logger1", soralog::Level::INFO);
  log_system.setSinkOfLogger("logger1", "sink1");

  // Reset some properties and inherent them from the parent group
  log_system.resetLevelOfGroup("logger1");
  log_system.resetSinkOfGroup("logger1");

  // Log message
  logger1->info("Hello, world!");
  logger1->warn("Console pwned!");
}
