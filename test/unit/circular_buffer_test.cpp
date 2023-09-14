/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#if __cplusplus > 201703L
#include <barrier>
#endif

#include "soralog/impl/sink_to_file.hpp"

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

class CircularBufferTest : public ::testing::Test {
 public:
  class Data : std::array<char, 10> {
   public:
    Data(char filler) {
      std::fill(begin(), end(), filler);
    }
    bool operator==(const Data &other) const {
      return (const std::array<char, 10> &)(*this)
          == (const std::array<char, 10> &)(other);
    }
    char c() const {
      return this->front();
    }
  };
};

TEST_F(CircularBufferTest, Create) {
  size_t capacity = 5;

  CircularBuffer<Data> testee(capacity);

  EXPECT_EQ(testee.size(), 0);
  EXPECT_EQ(testee.avail(), capacity);
  EXPECT_EQ(testee.capacity(), capacity);
}

TEST_F(CircularBufferTest, Put) {
  size_t capacity = 3;

  CircularBuffer<Data> testee(capacity);

  // Fill for full
  for (auto i = 0; i < capacity; ++i) {
    EXPECT_EQ(testee.size(), i);
    EXPECT_EQ(testee.avail(), capacity - i);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- put #" << (i + 1) << std::endl;
    auto ref = testee.put('1' + i);
    EXPECT_TRUE(ref);
  }

  EXPECT_EQ(testee.size(), capacity);
  EXPECT_EQ(testee.avail(), 0);
  EXPECT_EQ(testee.capacity(), capacity);

  // Overfill
  std::cout << "--- put #" << (capacity + 1) << " (overfill)" << std::endl;
  auto ref = testee.put('1' + capacity);
  EXPECT_FALSE(ref);

  EXPECT_EQ(testee.size(), capacity);
  EXPECT_EQ(testee.avail(), 0);
  EXPECT_EQ(testee.capacity(), capacity);
  std::cout << '\n' << std::endl;
}

TEST_F(CircularBufferTest, Get) {
  size_t capacity = 3;

  CircularBuffer<Data> testee(capacity);

  {  // Get when empty
    EXPECT_EQ(testee.size(), 0);
    EXPECT_EQ(testee.avail(), capacity);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- get (nothing actually)" << std::endl;
    auto ref = testee.get();
    EXPECT_FALSE(ref);
  }

  // Fill for full
  for (auto i = 0; i < capacity; ++i) {
    auto ref = testee.put('1' + i);
  }

  // Get for empty
  for (auto i = 0; i < capacity; ++i) {
    EXPECT_EQ(testee.size(), capacity - i);
    EXPECT_EQ(testee.avail(), i);
    EXPECT_EQ(testee.capacity(), capacity);

    std::cout << "--- get #" << (i + 1) << std::endl;
    auto ref = testee.get();
    EXPECT_TRUE(ref);

    EXPECT_EQ(*ref, Data('1' + i));
  }

  EXPECT_EQ(testee.size(), 0);
  EXPECT_EQ(testee.avail(), capacity);
  EXPECT_EQ(testee.capacity(), capacity);
}

TEST_F(CircularBufferTest, PutGet) {
  size_t capacity = 10;

  size_t i = 0;

  for (auto lag = 0; lag < capacity; ++lag) {
    CircularBuffer<Data> testee(capacity);
    for (auto n = 0; n < lag; ++n) {
      char c = '0' + (++i % capacity);
      auto ref_put = testee.put(c);
      EXPECT_TRUE(ref_put);
      std::cout << "[lag=" << lag << "]: "          //
                << "put " << ref_put->c() << " > "  //
                << "size=" << testee.size() << " avail=" << testee.avail()
                << std::endl;
    }
    for (auto n = 0; n < capacity; ++n) {
      {
        char c = '0' + (++i % capacity);
        auto ref_put = testee.put(c);
        EXPECT_TRUE(ref_put);
        std::cout << "[lag=" << lag << "]: "          //
                  << "put " << ref_put->c() << " > "  //
                  << "size=" << testee.size() << " avail=" << testee.avail()
                  << std::endl;
      }
      {
        auto ref_get = testee.get();
        EXPECT_TRUE(ref_get);
        std::cout << "[lag=" << lag << "]: "          //
                  << "get " << ref_get->c() << " > "  //
                  << "size=" << testee.size() << " avail=" << testee.avail()
                  << std::endl;
      }
    }
  }
}

TEST_F(CircularBufferTest, PutGetMt) {
  size_t capacity = 10;

  CircularBuffer<Data> testee(capacity);

  std::atomic_size_t i = 0;
  std::atomic_size_t n = 100;

#if __cplusplus > 201703L
  std::barrier barrier(3);
#endif

  std::thread prod([&] {
#if __cplusplus > 201703L
    barrier.arrive_and_wait();
#endif
    while (i < n) {
      std::cout << "w" << i << std::endl;
      //      if (auto ref = testee.put('0' + (i % 10))) {
      if (auto ref = testee.put('0')) {
        ++i;
        std::cout << "put " << ref->c()  //
                  << " [" << testee.size() << " | " << testee.avail() << "]"
                  << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(3 + random() % 50));
    }
  });

  std::thread cons([&] {
#if __cplusplus > 201703L
    barrier.arrive_and_wait();
#endif
    while (i < n) {
      std::cout << "r" << std::endl;
      if (auto ref = testee.get()) {
        std::cout << "get " << ref->c()  //
                  << " [" << testee.size() << " | " << testee.avail() << "]"
                  << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(3 + random() % 50));
    }
  });

#if __cplusplus > 201703L
  barrier.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}

TEST_F(CircularBufferTest, Mutual) {
  size_t capacity = 10;

  CircularBuffer<Data> testee(capacity);

#if __cplusplus > 201703L
  std::barrier barrier(3);
#endif

  std::thread prod([&] {
#if __cplusplus > 201703L
    barrier.arrive_and_wait();
#endif
    if (auto ref = testee.put('*')) {
      std::cout << "put " << ref->c()  //
                << " [" << testee.size() << " | " << testee.avail() << "]"
                << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  });

  std::thread cons([&] {
#if __cplusplus > 201703L
    barrier.arrive_and_wait();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (auto ref = testee.get()) {
      std::cout << "get " << ref->c()  //
                << " [" << testee.size() << " | " << testee.avail() << "]"
                << std::endl;
    }
  });

#if __cplusplus > 201703L
  barrier.arrive_and_wait();
#endif
  prod.join();
  cons.join();
}
