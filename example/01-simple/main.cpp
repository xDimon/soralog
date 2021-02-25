/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging_object.hpp"

#include <injector/injector.hpp>

int main() {
  auto injector = soralog::injector::makeInjector();

  auto main_log = injector.create<soralog::LoggerFactory &>().get(
      "main", "console", soralog::Level::DEBUG);

  main_log->info("Start");

  auto &object = injector.create<LoggingObject &>();

  object.method();

  main_log->info("Finish");

  return 0;
}
