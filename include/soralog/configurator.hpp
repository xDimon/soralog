/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace soralog {

  class LoggingSystem;

  /**
   * @class Configurator
   * @brief Interface for configuring the logging system.
   *
   * The configurator is responsible for setting up the logging system
   * using an external or embedded configuration. It adds sinks and
   * groups according to the specified configuration.
   */
  class Configurator {
   public:
    virtual ~Configurator() = default;

    /**
     * @struct Result
     * @brief Represents the result of applying a configuration.
     */
    struct Result {
      /// Indicates whether a critical error occurred.
      /// If true, logging may fail.
      bool has_error = false;
      /// Indicates whether a non-critical warning occurred.
      /// Logging can continue.
      bool has_warning = false;
      /// Message explaining any errors or warnings.
      std::string message{};
    };

    /**
     * @brief Applies the configuration to a logging system.
     * @param system Reference to the target logging system.
     * @return Result of the configuration process.
     */
    virtual Result applyOn(LoggingSystem &system) const = 0;
  };

}  // namespace soralog
