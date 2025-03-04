/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#if __cplusplus >= 202002L
#include <latch>
#endif

#include "soralog/impl/sink_to_file.hpp"

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

/**
 * @brief Tests for CircularBuffer implementation.
 *
 * This test suite verifies the behavior of CircularBuffer, including
 * basic operations, boundary conditions, and multi-threaded scenarios.
 */

/// @brief Test fixture for CircularBuffer
class CircularBufferTest : public ::testing::Test {
 public:
  /// @brief Data structure stored in CircularBuffer.
  class Data : public std::array<char, 10> {
   public:
    explicit Data(char filler) {
      std::fill(begin(), end(), filler);
    }
    bool operator==(const Data &other) const {
      return static_cast<const std::array<char, 10> &>(*this)
          == static_cast<const std::array<char, 10> &>(other);
    }
    char c() const {
      return this->front();
    }
  };
};

/**
 * @brief Test buffer creation.
 *
 * @given A newly created CircularBuffer with specified capacity.
 * @when The buffer is initialized.
 * @then The buffer should be empty and have full capacity available.
 */
TEST_F(CircularBufferTest, Create) {
  size_t capacity = 5;

  CircularBuffer<Data> testee(capacity);

  EXPECT_EQ(testee.size(), 0);
  EXPECT_EQ(testee.avail(), capacity);
  EXPECT_EQ(testee.capacity(), capacity);
}

/**
 * @brief Test inserting elements into the buffer.
 *
 * @given An empty CircularBuffer with limited capacity.
 * @when Elements are inserted up to its capacity.
 * @then The buffer should be full, and further insertions should fail.
 */
TEST_F(CircularBufferTest, Put) {
  size_t capacity = 3;

  CircularBuffer<Data> testee(capacity);

  // Fill for full
  for (auto i = 0; i < capacity; ++i) {
    EXPECT_EQ(testee.size(), i);
    EXPECT_EQ(testee.avail(), capacity - i);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- put #" << (i + 1) << '\n';
    auto ref = testee.put('1' + i);
    EXPECT_TRUE(ref);
  }

  // Buffer should be at full capacity
  EXPECT_EQ(testee.size(), capacity);
  EXPECT_EQ(testee.avail(), 0);
  EXPECT_EQ(testee.capacity(), capacity);

  // Attempt to overfill
  std::cout << "--- put #" << (capacity + 1) << " (overfill)" << '\n';
  auto ref = testee.put('1' + capacity);
  EXPECT_FALSE(ref);  // Should fail as buffer is full

  EXPECT_EQ(testee.size(), capacity);
  EXPECT_EQ(testee.avail(), 0);
  EXPECT_EQ(testee.capacity(), capacity);
  std::cout << '\n' << '\n';
}

/**
 * @brief Test retrieving elements from the buffer.
 *
 * @given A CircularBuffer with elements added.
 * @when Elements are retrieved.
 * @then Retrieved elements should match the order of insertion.
 */
TEST_F(CircularBufferTest, Get) {
  size_t capacity = 3;

  CircularBuffer<Data> testee(capacity);

  {  // Get when empty
    EXPECT_EQ(testee.size(), 0);
    EXPECT_EQ(testee.avail(), capacity);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- get (nothing actually)" << '\n';
    // Try getting from empty buffer
    auto ref = testee.get();
    EXPECT_FALSE(ref);  // Should return nothing
  }

  // Fill buffer to full
  for (size_t i = 0; i < capacity; ++i) {
    std::ignore = testee.put('1' + i);
  }

  // Retrieve elements and verify order
  for (size_t i = 0; i < capacity; ++i) {
    EXPECT_EQ(testee.size(), capacity - i);
    EXPECT_EQ(testee.avail(), i);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- get #" << (i + 1) << '\n';
    auto ref = testee.get();
    EXPECT_TRUE(ref);

    EXPECT_EQ(*ref, Data('1' + i));
  }

  // Buffer should be empty now
  EXPECT_EQ(testee.size(), 0);
  EXPECT_EQ(testee.avail(), capacity);
  EXPECT_EQ(testee.capacity(), capacity);
}

/**
 * @brief Test sequential put and get operations.
 *
 * @given A CircularBuffer with limited capacity.
 * @when Elements are added and removed in sequence.
 * @then The buffer should behave as expected, maintaining the correct state.
 */
TEST_F(CircularBufferTest, PutGet) {
  size_t capacity = 10;

  size_t i = 0;

  for (size_t lag = 0; lag < capacity; ++lag) {
    CircularBuffer<Data> testee(capacity);

    // Pre-fill buffer up to lag
    for (size_t n = 0; n < lag; ++n) {
      char c = '0' + (++i % capacity);
      auto ref_put = testee.put(c);
      EXPECT_TRUE(ref_put);
      std::cout << "[lag=" << lag << "]: "          //
                << "put " << ref_put->c() << " > "  //
                << "size=" << testee.size() << " avail=" << testee.avail()
                << '\n';
    }

    // Perform put/get operations in sequence
    for (auto n = 0; n < capacity; ++n) {
      {
        char c = '0' + (++i % capacity);
        auto ref_put = testee.put(c);
        EXPECT_TRUE(ref_put);
        std::cout << "[lag=" << lag << "]: "          //
                  << "put " << ref_put->c() << " > "  //
                  << "size=" << testee.size() << " avail=" << testee.avail()
                  << '\n';
      }
      {
        auto ref_get = testee.get();
        EXPECT_TRUE(ref_get);
        std::cout << "[lag=" << lag << "]: "          //
                  << "get " << ref_get->c() << " > "  //
                  << "size=" << testee.size() << " avail=" << testee.avail()
                  << '\n';
      }
    }
  }
}

/**
 * @brief Test concurrent access to the buffer (multi-threaded).
 *
 * @given A CircularBuffer shared between producer and consumer threads.
 * @when The producer adds data while the consumer retrieves it.
 * @then The buffer should remain consistent and no data should be lost.
 */
TEST_F(CircularBufferTest, PutGetMt) {
  size_t capacity = 10;

  CircularBuffer<Data> testee(capacity);

  std::atomic_size_t i = 0;
  std::atomic_size_t n = 100;

#if __cplusplus >= 202002L
  std::latch latch(3);
#endif

  std::thread prod([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    while (i < n) {
      std::cout << "w" << i << '\n';
      if (auto ref = testee.put('0')) {
        ++i;
        std::cout << "put " << ref->c()  //
                  << " [" << testee.size() << " | " << testee.avail() << "]"
                  << '\n';
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(3 + random() % 50));
    }
  });

  std::thread cons([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    while (i < n) {
      std::cout << "r" << '\n';
      if (auto ref = testee.get()) {
        std::cout << "get " << ref->c()  //
                  << " [" << testee.size() << " | " << testee.avail() << "]"
                  << '\n';
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(3 + random() % 50));
    }
  });

#if __cplusplus >= 202002L
  latch.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}

/**
 * @brief Test mutual exclusion during buffer operations.
 *
 * @given A CircularBuffer with a single producer and consumer.
 * @when The producer adds data and the consumer retrieves it with delays.
 * @then The buffer should correctly handle synchronization.
 */
TEST_F(CircularBufferTest, Mutual) {
  size_t capacity = 10;

  CircularBuffer<Data> testee(capacity);

#if __cplusplus >= 202002L
  std::latch latch(3);
#endif

  std::thread prod([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    if (auto ref = testee.put('*')) {
      std::cout << "put " << ref->c()  //
                << " [" << testee.size() << " | " << testee.avail() << "]"
                << '\n';
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });

  std::thread cons([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (auto ref = testee.get()) {
      std::cout << "get " << ref->c()  //
                << " [" << testee.size() << " | " << testee.avail() << "]"
                << '\n';
    }
  });

#if __cplusplus >= 202002L
  latch.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}
