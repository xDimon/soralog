/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/fallback_configurator.hpp>

#include <soralog/level.hpp>

#include <soralog/impl/sink_to_console.hpp>

namespace soralog {

  Configurator::Result FallbackConfigurator::applyOn(
      LoggingSystem &system) const {
    // Create a default console sink with the specified logging level and color
    // option.
    system.makeSink<SinkToConsole>(
        "console", level_, SinkToConsole::Stream::STDOUT, with_color_);

    // Create a default logging group "*" that routes all logs to the console
    // sink.
    system.makeGroup("*", {}, "console", level_);

    // Return a result indicating that a fallback configuration has been
    // applied.
    return {.has_error = false,
            .has_warning = true,
            .message = std::string()
                     + "I: Using fallback configurator for logger system\n"
                       "I: All logs will be written to "
                     + (with_color_ ? "color " : "") + "standard output with '"
                     + levelToStr(level_) + "' level"};
  }

}  // namespace soralog
