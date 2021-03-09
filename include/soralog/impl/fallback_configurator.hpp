/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_FALLBACKCONFIGURATOR
#define SORALOG_FALLBACKCONFIGURATOR

#include <soralog/configurator.hpp>

#include <soralog/logger_system.hpp>

namespace soralog {

  class FallbackConfigurator : public Configurator {
   public:
    ~FallbackConfigurator() override = default;

    Result applyOn(LoggerSystem &system) const override;

    void setLevel(Level level);
    void withColor(bool with_color);

   private:
    Level level_ = Level::INFO;
    bool with_color_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_FALLBACKCONFIGURATOR
