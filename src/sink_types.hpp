/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_SINKTYPES
#define SORALOG_SINKTYPES

namespace soralog {

  /**
   * Types of sink
   */
  enum class SinkType {
    COUT,  /// Writing messages to standard output
    FILE,  /// Wtiting messages to file
  };

}  // namespace soralog

#endif  // SORALOG_SINKTYPES
