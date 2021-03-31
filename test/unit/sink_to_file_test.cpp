/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "impl/sink_to_file.hpp"

using namespace soralog;
using namespace testing;

class SinkToFileTest : public ::testing::Test {
 public:
  struct FakeLogger {
    FakeLogger(std::shared_ptr<SinkToFile> sink) : sink_(std::move(sink)) {}

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
    char filename[L_tmpnam];
    path_ = std::filesystem::temp_directory_path();
    ASSERT_TRUE(std::tmpnam(filename) != nullptr);
    path_ /= std::string(filename) + ".log";

    sink_ = std::make_shared<SinkToFile>(
        "file", path_,
        Sink::ThreadInfoType::NONE,  // ignore thread info
        4,                           // capacity: 4 events
        16384,                       // buffers size: 16 Kb
        10);                         // latency: 200 ms
    logger_ = std::make_shared<FakeLogger>(sink_);
  }
  void TearDown() override {
    std::remove(path_.native().data());
  }

  std::chrono::milliseconds latency_{10};  // latency: 10 ms
  std::filesystem::path path_;
  std::shared_ptr<SinkToFile> sink_;
  std::shared_ptr<FakeLogger> logger_;
};

TEST_F(SinkToFileTest, Logging) {
  auto delay = std::chrono::milliseconds(5);
  logger_->debug("Uno");
  std::this_thread::sleep_for(delay * 1);
  logger_->debug("Dos");
  std::this_thread::sleep_for(delay * 2);
  logger_->debug("Tres");
  std::this_thread::sleep_for(delay * 3);
  logger_->debug("Cuatro");
  std::this_thread::sleep_for(delay * 4);
  logger_->flush();
}
