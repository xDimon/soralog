/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_INJECTOR
#define SORALOG_INJECTOR

#include <boost/di.hpp>

#include <soralog/logger_factory.hpp>
#include <soralog/logger_system.hpp>

namespace soralog::injector {

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    return di::make_injector<InjectorConfig>(

        // clang-format off
        di::bind<LoggerFactory>.template to<LoggerSystem>().in(di::singleton),
        // clang-format on

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace soralog::injector

#endif  // SORALOG_INJECTOR
