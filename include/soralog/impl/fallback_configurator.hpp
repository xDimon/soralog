/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_FALLBACKCONFIGURATOR
#define SORALOG_FALLBACKCONFIGURATOR

#include <soralog/configurator.hpp>

#include <soralog/logging_system.hpp>

namespace soralog {

  /**
   * @class FallbackConfigurator
   * @brief Fallback configurator of Logging System. Creates just one sink (to
   * console) and default group '*'. Constructor accepts detalisation level and
   * flag to shitch on color in console
   */
  class FallbackConfigurator : public Configurator {
   public:
    explicit FallbackConfigurator(Level level = Level::INFO,
                                  bool with_color = false)
        : level_(level), with_color_(with_color) {}

    ~FallbackConfigurator() override = default;

    Result applyOn(LoggingSystem &system) const override;

   private:
    Level level_ = Level::INFO;
    bool with_color_ = false;
  };

}  // namespace soralog

#endif  // SORALOG_FALLBACKCONFIGURATOR
