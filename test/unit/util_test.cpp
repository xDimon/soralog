/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <soralog/util.hpp>

#include <string>
#include <thread>
#include <utility>

using soralog::util::getThreadName;
using soralog::util::getThreadNumber;
using soralog::util::setThreadName;

namespace {

  // Platforms where setThreadName() names the thread for real
  constexpr bool kNamesThreads =
#if defined(__linux__) or defined(__APPLE__) or defined(__FreeBSD__) \
    or defined(__DragonFly__) or defined(__NetBSD__) or defined(__OpenBSD__)
      true;
#else
      false;
#endif

  // Runs the function in a new thread: the name is cached per thread on the
  // first getThreadName()
  template <typename F>
  void inNewThread(F &&f) {
    std::thread thread(std::forward<F>(f));
    thread.join();
  }

}  // namespace

/**
 * @test SetAndGet
 * @brief Tests that the name set to a thread is read back.
 *
 * @given A new thread.
 * @when The thread is named, then its name is read.
 * @then The name read is the name set.
 */
TEST(UtilTest, SetAndGet) {
  if (not kNamesThreads) {
    GTEST_SKIP() << "setThreadName() is not implemented for this platform";
  }
  std::string name;
  inNewThread([&] {
    setThreadName("soralog-test");
    name = getThreadName();
  });
  EXPECT_EQ(name, "soralog-test");
}

/**
 * @test LongName
 * @brief Tests that a long name is cut to 15 characters.
 *
 * @given A new thread.
 * @when The thread is named with more than 15 characters.
 * @then The name read is the first 15 characters.
 */
TEST(UtilTest, LongName) {
  if (not kNamesThreads) {
    GTEST_SKIP() << "setThreadName() is not implemented for this platform";
  }
  std::string name;
  inNewThread([&] {
    setThreadName("0123456789abcdefghij");
    name = getThreadName();
  });
  EXPECT_EQ(name, "0123456789abcde");
}

/**
 * @test ThreadNumber
 * @brief Tests that threads get different numbers, stable within a thread.
 *
 * @given The current thread and a new one.
 * @when Their numbers are read.
 * @then The numbers differ; a thread reads the same number twice.
 */
TEST(UtilTest, ThreadNumber) {
  auto own = getThreadNumber();
  EXPECT_EQ(getThreadNumber(), own);
  size_t other = 0;
  inNewThread([&] { other = getThreadNumber(); });
  EXPECT_NE(other, own);
}
