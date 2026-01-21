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

    MOCK_METHOD(void,
                prepare,
                (LoggingSystem & system, size_t index, Result &result),
                (override));
    MOCK_METHOD(void, applySinks, (), (const, override));
    MOCK_METHOD(void, applyGroups, (), (const, override));
    MOCK_METHOD(void, cleanup, (), (override));
  };

}  // namespace soralog
