/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/fallback_configurator.hpp>

#include <soralog/level.hpp>

#include "sink_to_console.hpp"

namespace soralog {

  Configurator::Result FallbackConfigurator::applyOn(
      LoggerSystem &system) const {
    system.makeSink<SinkToConsole>("*", true);
    system.makeGroup("", {}, "*", Level::TRACE);

    return {.has_error = false,
            .has_warning = true,
            .message = "I: Using fallback configurator of logger system."};
  }

}  // namespace soralog
