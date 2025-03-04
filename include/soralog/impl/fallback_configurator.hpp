/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/configurator.hpp>
#include <soralog/logging_system.hpp>

namespace soralog {

  /**
   * @class FallbackConfigurator
   * @brief Default configurator for the logging system.
   *
   * This configurator sets up a minimal logging system with:
   * - A single sink that outputs to the console.
   * - A default logging group named `*`.
   *
   * The constructor allows setting the logging level and enabling
   * color output in the console.
   */
  class FallbackConfigurator : public Configurator {
   public:
    /**
     * @brief Constructs a fallback configurator.
     * @param level Logging level (default: INFO).
     * @param with_color Enables color output in the console if true.
     */
    explicit FallbackConfigurator(Level level = Level::INFO,
                                  bool with_color = false)
        : level_(level), with_color_(with_color) {}

    ~FallbackConfigurator() override = default;

    /**
     * @brief Applies the fallback configuration to the logging system.
     * @param system Reference to the logging system.
     * @return Result of the configuration process.
     */
    Result applyOn(LoggingSystem &system) const override;

   private:
    Level level_ = Level::INFO;  ///< Default logging level.
    bool with_color_ = false;    ///< Enables colored console output.
  };

}  // namespace soralog
