/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "logger_factory.hpp"

namespace xlog {

  LoggerFactory::LoggerFactory(LoggerSystem &logger_system)
      : logger_system_(logger_system) {}

}  // namespace xlog
