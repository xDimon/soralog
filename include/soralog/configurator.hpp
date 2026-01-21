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
   *
   * Configuration may be composed from multiple configurators. They are applied
   * sequentially; later configurators may extend and/or override settings
   * produced by earlier ones.
   *
   * The configuration process is performed in multiple waves. In each wave,
   * the logging system iterates over the configurators in the same order:
   * - prepare(): read/parse configuration and perform any preprocessing
   * - applySinks(): apply sink-related configuration
   * - applyGroups(): apply group-related configuration
   * - cleanup(): release temporary resources and perform finalization
   *
   * Implementations are expected to be reusable and should tolerate being
   * executed as part of a chain of configurators.
   */
  class Configurator {
   public:
    virtual ~Configurator() = default;

    /**
     * @struct Result
     * @brief Represents the result of applying a configuration.
     *
     * Used by a configurator to report diagnostics to the caller without
     * throwing exceptions. The meaning and severity are:
     * - has_error: critical issue; configuration may be incomplete or invalid
     * - has_warning: non-critical issue; configuration can continue
     *
     * The message is expected to be human-readable.
     */
    struct Result {
      /// Indicates whether a critical error occurred.
      /// If true, logging may fail.
      bool has_error = false;
      /// Indicates whether a non-critical warning occurred.
      /// Logging can continue.
      bool has_warning = false;
      /// Message explaining any errors or warnings.
      std::string message;
    };

    /**
     * @brief Prepares this configurator for subsequent application.
     *
     * This method is called during the "prepare" wave for every configurator
     * in the chain. Typical responsibilities:
     * - read configuration sources (files, env vars, embedded defaults)
     * - parse and validate configuration
     * - store intermediate state needed for applySinks()/applyGroups()
     *
     * @param system Logging system being configured.
     * @param index Position of this configurator in the chain (0-based).
     * @param result Output structure for reporting errors/warnings.
     *
     * @note
     * The function should not assume it is the only configurator. It should
     * avoid destructive changes that prevent later configurators from applying
     * overrides.
     */
    virtual void prepare(LoggingSystem &system, int index, Result &result) = 0;

    /**
     * @brief Applies sink configuration.
     *
     * Called during the "apply sinks" wave after all configurators have been
     * prepared. Implementations should configure sinks in the logging system
     * according to the state gathered in prepare().
     *
     * This method is const: all required information should already be stored
     * inside the configurator instance.
     */
    virtual void applySinks() const = 0;

    /**
     * @brief Applies group configuration.
     *
     * Called during the "apply groups" wave after sinks have been applied.
     * Implementations should configure groups (e.g., default levels, sink
     * assignments, hierarchy) according to the state gathered in prepare().
     *
     * This method is const: all required information should already be stored
     * inside the configurator instance.
     */
    virtual void applyGroups() const = 0;

    /**
     * @brief Cleans up temporary resources after configuration is applied.
     *
     * Called during the final "cleanup" wave. Typical responsibilities:
     * - release cached/parsing resources
     * - close files / drop temporary buffers
     * - reset transient state so the configurator can be reused
     */
    virtual void cleanup() = 0;
  };

}  // namespace soralog