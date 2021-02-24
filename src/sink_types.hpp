/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef XLOG_SINKTYPES
#define XLOG_SINKTYPES

namespace xlog {

  /**
   * Types of sink
   */
  enum class SinkType {
    BLACKHOLE = 0,  /// It's dummy sink which eating all message without save
    COUT,           /// Writing messages to standard output
    FILE,           /// Wtiting messages to file
    UDP             /// Transmitting messages into network by UDP protocol
  };

}  // namespace xlog

#endif  // XLOG_SINKTYPES
