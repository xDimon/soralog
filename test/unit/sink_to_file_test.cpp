/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "soralog/impl/sink_to_file.hpp"

using namespace soralog;
using namespace testing;
using namespace std::chrono_literals;

class SinkToFileTest : public ::testing::Test {
 public:
  struct FakeLogger {
    explicit FakeLogger(std::shared_ptr<SinkToFile> sink)
        : sink_(std::move(sink)) {}

    template <typename... Args>
    void debug(std::string_view format, const Args &... args) {
      sink_->push("logger", Level::DEBUG, format, args...);
    }

    void flush() {
      sink_->flush();
    }

   private:
    std::shared_ptr<SinkToFile> sink_;
  };

  void SetUp() override {
    std::array<char, L_tmpnam> filename{};
    path_ = std::filesystem::temp_directory_path();
    ASSERT_TRUE(std::tmpnam(filename.data()) != nullptr);
    path_ /= std::string(filename.data()) + ".log";
  }
  void TearDown() override {
    std::remove(path_.native().data());
  }

  std::shared_ptr<FakeLogger> createLogger(std::chrono::milliseconds latency) {
    auto sink = std::make_shared<SinkToFile>(
        "file", path_,
        Sink::ThreadInfoType::NONE,  // ignore thread info
        4,                           // capacity: 4 events
        16384,                       // buffers size: 16 Kb
        latency.count());
    return std::make_shared<FakeLogger>(std::move(sink));
  }

 private:
  std::filesystem::path path_;
};

TEST_F(SinkToFileTest, Logging) {
  auto logger = createLogger(20ms);
  auto delay = 1ms;
  int count = 100;
  for (int round = 1; round <= 3; ++round) {
    for (int i = 1; i <= count; ++i) {
      logger->debug("round: {}, message: {}, delay: {}ms", round, i,
                    abs(i - count / 2));
      std::this_thread::sleep_for(delay * abs(i - count / 2));
    }
  }
  logger->flush();
}
