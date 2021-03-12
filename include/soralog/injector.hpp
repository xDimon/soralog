/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_INJECTOR
#define SORALOG_INJECTOR

#include <boost/di.hpp>

#include <soralog/impl/fallback_configurator.hpp>
#include <soralog/logging_system.hpp>
#include <soralog/macro.hpp>

namespace soralog::injector {

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    return di::make_injector<InjectorConfig>(

        // clang-format off
        di::bind<Configurator>.template to<FallbackConfigurator>().in(di::unique),
        di::bind<LoggerFactory>.template to<LoggingSystem>().in(di::singleton),
        // clang-format on

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace soralog::injector

#endif  // SORALOG_INJECTOR
