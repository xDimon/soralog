/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
