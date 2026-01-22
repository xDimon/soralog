/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Example: manual (code-driven) configuration of the logging system.
 *
 * This example demonstrates how to configure soralog without external
 * configurators:
 * - create sinks and groups programmatically
 * - set a fallback group
 * - override and reset group/logger properties
 *
 * Note: a default-constructed LoggingSystem is already configured and provides
 * a builtin sink "*" (sink-to-nowhere) and a builtin group "*".
 */

#include <soralog/group.hpp>
#include <soralog/impl/sink_to_console.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

int main() {
  // Create the logging system (already configured, no configurators).
  // The builtin "*" sink/group are created by the constructor.
  soralog::LoggingSystem log_system;

  // Manual creation of a sink
  log_system.makeSink<soralog::SinkToConsole>(
      "sink1",
      soralog::Level::INFO,
      soralog::SinkToConsole::Stream::STDOUT,
      true);

  // Manual creation of a group
  auto group1 =
      log_system.makeGroup("group1", {}, "sink1", soralog::Level::INFO);

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

  // Reset some properties and inherit them from the parent group.
  group2->resetLevel();
  group2->resetSink();

  // Forget a parent group and become independent
  group2->unsetParentGroup();

  // Manual post-setup of the group over logging system
  log_system.setParentOfGroup("group2", "group1");
  log_system.setLevelOfGroup("group2", soralog::Level::INFO);
  log_system.setSinkOfGroup("group2", "sink1");

  // Reset some properties and inherit them from the parent group
  log_system.resetLevelOfGroup("group2");
  log_system.resetSinkOfGroup("group2");

  // Create logger and inherit setting from a specified group
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

  // Reset some properties and inherit them from the parent group
  log_system.resetLevelOfLogger("logger1");
  log_system.resetSinkOfLogger("logger1");

  // Log message
  logger1->info("Hello, world!");
  logger1->warn("Console pwned!");

  // Demonstrate an explicit per-logger level override.
  logger3->trace("This trace is visible only if group/logger level allows it");
}
