/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <sys/resource.h>

#include <thread>

#include "soralog/impl/sink_to_syslog.hpp"

using namespace soralog;
using namespace std::chrono_literals;

namespace {
  /// @brief CPU time consumed by the whole process
  std::chrono::microseconds processCpuTime() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return std::chrono::seconds(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec)
         + std::chrono::microseconds(usage.ru_utime.tv_usec
                                     + usage.ru_stime.tv_usec);
  }
}  // namespace

/**
 * @test IdleWorkerDoesNotSpin
 * @brief Tests that the worker thread of an idle syslog sink sleeps.
 *
 * @given A syslog sink with small latency and nothing to log.
 * @when Time passes.
 * @then The process consumes almost no CPU time.
 */
TEST(SinkToSyslogTest, IdleWorkerDoesNotSpin) {
  auto sink =
      std::make_shared<SinkToSyslog>("syslog",
                                     Level::TRACE,
                                     "soralog_test",
                                     Sink::ThreadInfoType::NONE,
                                     4,      // Capacity: 4 events
                                     64,     // Max message length: 64 bytes
                                     16384,  // Buffer size: 16 KB
                                     10);    // Log flush latency: 10 ms
  std::this_thread::sleep_for(50ms);         // Let the worker start

  auto wall = 500ms;
  auto before = processCpuTime();
  std::this_thread::sleep_for(wall);
  auto spent = processCpuTime() - before;

  EXPECT_LT(spent, wall / 5);
}
