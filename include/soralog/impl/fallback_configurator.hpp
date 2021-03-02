/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_FALLBACKCONFIGURATOR
#define SORALOG_FALLBACKCONFIGURATOR

#include <soralog/configurator.hpp>

#include <soralog/logger_system.hpp>

namespace soralog {

  class FallbackConfigurator final : public Configurator {
   public:
    ~FallbackConfigurator() override = default;

    Result applyOn(LoggerSystem &system) const override;
  };

}  // namespace soralog

#endif  // SORALOG_FALLBACKCONFIGURATOR
