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
    BLACKHOLE = 0,  /// It's dummy sink which eating all message without save
    COUT,           /// Writing messages to standard output
    FILE,           /// Wtiting messages to file
  };

}  // namespace soralog

#endif  // SORALOG_SINKTYPES
