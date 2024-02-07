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

  class ConfiguratorMock : public Configurator {
   public:
    ~ConfiguratorMock() override = default;

    MOCK_CONST_METHOD1(applyOn, Result(LoggingSystem &));
  };

}  // namespace soralog
