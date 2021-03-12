/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/fallback_configurator.hpp>

#include <soralog/level.hpp>

#include "sink_to_console.hpp"

namespace soralog {

  Configurator::Result FallbackConfigurator::applyOn(
      LoggingSystem &system) const {
    system.makeSink<SinkToConsole>("console", with_color_);
    system.makeGroup("*", {}, "console", level_);

    return {.has_error = false,
            .has_warning = true,
            .message =
                "I: Using fallback configurator for logger system\n"
                "I: All logs will be write into console with 'INFO' level"};
  }

  void FallbackConfigurator::setLevel(Level level) {
    level_ = level;
  }

  void FallbackConfigurator::withColor(bool with_color) {
    with_color_ = with_color;
  }

}  // namespace soralog
