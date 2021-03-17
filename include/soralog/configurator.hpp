/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CONFIGURATOR
#define SORALOG_CONFIGURATOR

#include <string>

namespace soralog {

  class LoggingSystem;

  /**
   * @interface Configurator
   * @brief This object serves for set up Logging System in according with
   * config (external or embedded) by adding of sinks and groups.
   */
  class Configurator {
   public:
    virtual ~Configurator() = default;

    /**
     * Result of applying config by configurator
     */
    struct Result {
      /// Error flag. Set to true if has error, and logging can't work correctly
      bool has_error = false;
      /// Warning flag. Set to true if has error, but logging can work
      bool has_warning = false;
      /// Messages for explaining of error and warning
      std::string message{};
    };

    /**
     * @param system is reference to target LoggingSystem
     * @return result of applying
     */
    virtual Result applyOn(LoggingSystem &system) const = 0;
  };

}  // namespace soralog

#endif  // SORALOG_CONFIGURATOR
