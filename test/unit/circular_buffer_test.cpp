/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#if __cplusplus >= 202002L
#include <latch>
#endif

#include "soralog/circular_buffer.hpp"

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

namespace {
  /// Random delay between operations of the multi-threaded test
  std::chrono::milliseconds randomDelay() {
    thread_local std::minstd_rand rng(std::random_device{}());
    return std::chrono::milliseconds(3 + rng() % 50);
  }
}  // namespace

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

  /// @brief One for all instantiations of print().
  inline static std::mutex print_mutex;

  /// @brief Prints a line; serialized, as some tests print from two threads.
  template <typename... Args>
  static void print(const Args &...args) {
    std::lock_guard lock(print_mutex);
    (std::cout << ... << args) << '\n';
  }
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
 * @brief Test creating a buffer of zero capacity.
 *
 * @given Zero capacity.
 * @when A CircularBuffer is constructed.
 * @then Construction fails with std::invalid_argument.
 */
TEST_F(CircularBufferTest, ZeroCapacity) {
  EXPECT_THROW(CircularBuffer<Data> testee(0), std::invalid_argument);
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
  for (size_t i = 0; i < capacity; ++i) {
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
 * @then Concurrent operations complete without errors (data races are
 *       caught by sanitizers).
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
      print("w", i);
      if (auto ref = testee.put('0')) {
        ++i;
        print(
            "put ", ref->c(), " [", testee.size(), " | ", testee.avail(), "]");
      }
      std::this_thread::sleep_for(randomDelay());
    }
  });

  std::thread cons([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    while (i < n) {
      print("r");
      if (auto ref = testee.get()) {
        print(
            "get ", ref->c(), " [", testee.size(), " | ", testee.avail(), "]");
      }
      std::this_thread::sleep_for(randomDelay());
    }
  });

#if __cplusplus >= 202002L
  latch.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}

/**
 * @brief Test that an element is not consumed before it is published.
 *
 * @given A CircularBuffer with a single producer and consumer.
 * @when The producer holds its element unpublished for a second, and the
 *       consumer tries to get it meanwhile.
 * @then The consumer gets nothing.
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
      print("put ", ref->c(), " [", testee.size(), " | ", testee.avail(), "]");
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });

  std::thread cons([&] {
#if __cplusplus >= 202002L
    latch.arrive_and_wait();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto ref = testee.get();
    EXPECT_FALSE(ref);  // Not published while the producer holds it
  });

#if __cplusplus >= 202002L
  latch.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}

/**
 * @brief Test put_wait() on a full buffer (single-threaded).
 *
 * @given A full CircularBuffer.
 * @when put_wait() is called, and its callback gets one element.
 * @then The callback is called until a slot is free, the new element is
 *       stored after the old ones, and the order is kept.
 */
TEST_F(CircularBufferTest, PutWaitFull) {
  size_t capacity = 4;

  CircularBuffer<Data> testee(capacity);

  size_t calls = 0;
  auto never = [&](size_t) { ++calls; };
  for (size_t i = 0; i < capacity; ++i) {
    auto ref = testee.put_wait(never, static_cast<char>('a' + i));
    EXPECT_EQ(ref->c(), 'a' + i);
  }
  EXPECT_EQ(calls, 0);
  EXPECT_EQ(testee.size(), capacity);

  // The buffer is full: the callback must free a slot
  std::string got;
  auto drain_one = [&](size_t attempt) {
    ++calls;
    if (attempt == 2) {  // let it spin a little first
      auto ref = testee.get();
      ASSERT_TRUE(ref);
      got += ref->c();
    }
  };
  {
    auto ref = testee.put_wait(drain_one, 'e');
    EXPECT_EQ(ref->c(), 'e');
  }
  EXPECT_EQ(calls, 3);
  EXPECT_EQ(got, "a");

  while (auto ref = testee.get()) {
    got += ref->c();
  }
  EXPECT_EQ(got, "abcde");
}

/**
 * @brief Test put_wait() with several producers and a consumer.
 *
 * @given A small CircularBuffer, several producers and one consumer.
 * @when Each producer puts its numbered items with put_wait(), waiting for
 *       the consumer when the buffer is full.
 * @then The consumer gets every item once, the items of each producer in
 *       the order they were put (data races are caught by sanitizers).
 */
TEST_F(CircularBufferTest, PutWaitMt) {
  struct Item {
    Item(uint32_t producer, uint32_t seq) : producer(producer), seq(seq) {}
    uint32_t producer;
    uint32_t seq;
  };

  constexpr size_t capacity = 8;
  constexpr uint32_t producers = 4;
  constexpr uint32_t items = 20000;

  CircularBuffer<Item> testee(capacity);

  std::vector<std::thread> threads;
  threads.reserve(producers);
  for (uint32_t p = 0; p < producers; ++p) {
    threads.emplace_back([&, p] {
      for (uint32_t i = 0; i < items; ++i) {
        auto ref =
            testee.put_wait([](size_t) { std::this_thread::yield(); }, p, i);
      }
    });
  }

  std::vector<uint32_t> next(producers, 0);
  size_t total = 0;
  bool ordered = true;
  while (total < size_t{producers} * items) {
    if (auto ref = testee.get()) {
      ordered = ordered and ref->seq == next[ref->producer];
      next[ref->producer] = ref->seq + 1;
      ++total;
    } else {
      std::this_thread::yield();
    }
  }
  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(ordered);
  for (uint32_t p = 0; p < producers; ++p) {
    EXPECT_EQ(next[p], items);
  }
  EXPECT_FALSE(testee.get());
}
