/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CONFIGURATOR
#define SORALOG_CONFIGURATOR

#include <string>

namespace soralog {
  class LoggingSystem;

  class Configurator {
   public:
    virtual ~Configurator() = default;

    struct Result {
      bool has_error = false;
      bool has_warning = false;
      std::string message{};
    };

    virtual Result applyOn(LoggingSystem &system) const = 0;
  };

}  // namespace soralog

#endif  // SORALOG_CONFIGURATOR
