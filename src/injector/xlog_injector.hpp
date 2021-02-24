/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_INJECTOR
#define XLOG_INJECTOR

#include <boost/di.hpp>

namespace xlog::injector {

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    return di::make_injector<InjectorConfig>(

        // clang-format off
// . . . .
        // clang-format on

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }

}  // namespace xlog::injector

#endif  // XLOG_INJECTOR
