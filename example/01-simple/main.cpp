/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging_object.hpp"

#include <configurator.hpp>
#include <injector/injector.hpp>

int main() {
  auto injector = soralog::injector::makeInjector();

  auto &configurator = injector.create<soralog::Configurator &>();

  configurator.loadFromFile(
      "/home/di/Projects/xLog/example/01-simple/logger.yml");

  auto main_log = injector.create<soralog::LoggerFactory &>().getLogger(
      "main", "main", soralog::Level::DEBUG);

  main_log->info("Start");

  auto &object = injector.create<LoggingObject &>();

  object.method();

  main_log->info("Finish");

  return 0;
}
