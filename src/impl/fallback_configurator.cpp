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

  void FallbackConfigurator::prepare(LoggingSystem &system,
                                     size_t index,
                                     Result &result) {
    applicator_ =
        std::make_tuple(std::ref(system), index + 1, std::ref(result));
  }

  void FallbackConfigurator::applySinks() const {
    if (applicator_.has_value()) {
      // Create a default console sink with the specified logging level and
      // color option.
      std::get<0>(applicator_.value())
          .get()
          .makeSink<SinkToConsole>(
              "console", level_, SinkToConsole::Stream::STDOUT, with_color_);
    }
  }

  void FallbackConfigurator::applyGroups() const {
    if (applicator_.has_value()) {
      // Create a default logging group "*" that routes all logs to the console
      // sink.
      std::get<0>(applicator_.value())
          .get()
          .makeGroup("*", {}, "console", level_);
    }
  }

  void FallbackConfigurator::cleanup() {
    if (applicator_.has_value()) {
      // Set a result indicating that a fallback configuration has been applied.
      auto id = std::get<1>(applicator_.value());
      auto &result = std::get<2>(applicator_.value()).get();
      result.has_warning = true;
      result.message += fmt::format(
          "I{}: Using fallback configurator for logger system\n", id);
      result.message += fmt::format(
          "I{}: All logs will be written to {}standard output with '{}' "
          "level\n",
          id,
          with_color_ ? "color " : "",
          levelToStr(level_));
      applicator_.reset();
    }
  };

}  // namespace soralog
