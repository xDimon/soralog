/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logging_object.hpp"

#include <injector/xlog_injector.hpp>

int main() {
  auto injector = xlog::injector::makeInjector();

  auto main_log = injector.create<xlog::LoggerFactory &>().get(
      "main", "console", xlog::Level::DEBUG);

  main_log->info("Start");

  auto &object = injector.create<LoggingObject &>();

  object.method();

  main_log->info("Finish");

  return 0;
}
