/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include <soralog/configurator.hpp>

namespace soralog {

  /**
   * @class ConfiguratorMock
   * @brief Mock implementation of the Configurator class for testing.
   *
   * This class is used to simulate a Configurator in unit tests.
   * Instead of actually applying configuration to the logging system,
   * it allows verification of calls to `applyOn()` with expected parameters.
   */
  class ConfiguratorMock : public Configurator {
   public:
    /// Default destructor.
    ~ConfiguratorMock() override = default;

    /**
     * @brief Mocks the applyOn method.
     *
     * This method does not modify the `LoggingSystem`. Instead, it
     * allows tracking and verifying that `applyOn` was called with
     * the expected arguments.
     *
     * @param system The logging system to which configuration should be
     * applied.
     * @return A `Result` structure indicating the success or failure of the
     * configuration.
     */
    MOCK_METHOD(Result, applyOn, (LoggingSystem &), (const, override));
  };

}  // namespace soralog
