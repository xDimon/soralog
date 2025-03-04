/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <mock/configurator_mock.hpp>
#include <mock/sink_mock.hpp>
#include <soralog/group.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

using namespace soralog;
using namespace testing;

/**
 * @class LoggerTest
 * @brief Test fixture for testing Logger functionality in LoggingSystem.
 *
 * This fixture initializes a logging system with multiple groups, sinks,
 * and loggers for testing various configurations and modifications of loggers.
 */
class LoggerTest : public ::testing::Test {
 public:
  void SetUp() override {
    configurator_ = std::make_shared<ConfiguratorMock>();
    system_ = std::make_shared<LoggingSystem>(configurator_);

    ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
      return &s == system_.get();
    }))).WillByDefault(Invoke([&](LoggingSystem &system) {
      system.makeSink<SinkMock>("sink1");
      system.makeSink<SinkMock>("sink2");
      system.makeSink<SinkMock>("sink3");
      system.makeSink<SinkMock>("sink4");
      system.makeGroup("first", {}, "sink1", Level::TRACE);
      system.makeGroup("second", {}, "sink2", Level::DEBUG);
      return Configurator::Result{};
    }));
    EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

    EXPECT_NO_THROW(auto r = system_->configure());

    ASSERT_TRUE(group1_ = system_->getGroup("first"));
    ASSERT_TRUE(group2_ = system_->getGroup("second"));

    ASSERT_TRUE(sink1_ = system_->getSink("sink1"));
    ASSERT_TRUE(sink2_ = system_->getSink("sink2"));
    ASSERT_TRUE(sink3_ = system_->getSink("sink3"));
    ASSERT_TRUE(sink4_ = system_->getSink("sink4"));

    ASSERT_TRUE(log1_ = system_->getLogger("log1", "first"));
    ASSERT_TRUE(log2_ = system_->getLogger("log2", "first", "sink3"));
    ASSERT_TRUE(log3_ = system_->getLogger("log3", "first", Level::INFO));
    ASSERT_TRUE(
        log4_ = system_->getLogger("log4", "first", "sink4", Level::VERBOSE));
  }
  void TearDown() override {}

 protected:
  std::shared_ptr<ConfiguratorMock> configurator_;
  std::shared_ptr<LoggingSystem> system_;
  std::shared_ptr<Group> group1_;
  std::shared_ptr<Group> group2_;
  std::shared_ptr<Sink> sink1_;
  std::shared_ptr<Sink> sink2_;
  std::shared_ptr<Sink> sink3_;
  std::shared_ptr<Sink> sink4_;
  std::shared_ptr<Logger> log1_;
  std::shared_ptr<Logger> log2_;
  std::shared_ptr<Logger> log3_;
  std::shared_ptr<Logger> log4_;
};

/**
 * @test MakeLogger
 * @brief Tests logger creation and property inheritance.
 *
 * @given A logging system with multiple groups and sinks.
 * @when Loggers are created with or without explicitly set properties.
 * @then Loggers inherit properties from their group unless explicitly
 * overridden.
 */
TEST_F(LoggerTest, MakeLogger) {
  // Verify loggers with default properties from their groups
  EXPECT_TRUE(log1_->group() == group1_);
  EXPECT_TRUE(log1_->level() == Level::TRACE);
  EXPECT_FALSE(log1_->isLevelOverridden());
  EXPECT_TRUE(log1_->sink() == sink1_);
  EXPECT_FALSE(log1_->isSinkOverridden());

  // Logger with explicitly set sink
  EXPECT_TRUE(log2_->group() == group1_);
  EXPECT_TRUE(log2_->level() == Level::TRACE);
  EXPECT_FALSE(log2_->isLevelOverridden());
  EXPECT_TRUE(log2_->sink() == sink3_);
  EXPECT_TRUE(log2_->isSinkOverridden());

  // Logger with explicitly set level
  EXPECT_TRUE(log3_->group() == group1_);
  EXPECT_TRUE(log3_->level() == Level::INFO);
  EXPECT_TRUE(log3_->isLevelOverridden());
  EXPECT_TRUE(log3_->sink() == sink1_);
  EXPECT_FALSE(log3_->isSinkOverridden());

  // Logger with explicitly set sink and level
  EXPECT_TRUE(log4_->group() == group1_);
  EXPECT_TRUE(log4_->level() == Level::VERBOSE);
  EXPECT_TRUE(log4_->isLevelOverridden());
  EXPECT_TRUE(log4_->sink() == sink4_);
  EXPECT_TRUE(log4_->isSinkOverridden());
}

/**
 * @test ChangeLevel
 * @brief Tests changing and resetting logging levels in loggers.
 *
 * @given A logging system with a hierarchical group structure.
 * @when Levels are explicitly set for some loggers.
 * @then The new level is applied and marked as overridden.
 * @when The levels are reset.
 * @then The level is inherited from the group and marked as not overridden.
 */
TEST_F(LoggerTest, ChangeLevel) {
  // Set custom logging levels for loggers
  log1_->setLevel(Level::CRITICAL);
  log2_->setLevel(Level::CRITICAL);
  log3_->setLevel(Level::CRITICAL);
  log4_->setLevel(Level::CRITICAL);

  // Verify that the levels are set and marked as overridden
  EXPECT_TRUE(log1_->level() == Level::CRITICAL);
  EXPECT_TRUE(log1_->isLevelOverridden());
  EXPECT_TRUE(log2_->level() == Level::CRITICAL);
  EXPECT_TRUE(log2_->isLevelOverridden());
  EXPECT_TRUE(log3_->level() == Level::CRITICAL);
  EXPECT_TRUE(log3_->isLevelOverridden());
  EXPECT_TRUE(log4_->level() == Level::CRITICAL);
  EXPECT_TRUE(log4_->isLevelOverridden());

  // Reset levels back to group values
  log1_->resetLevel();
  log2_->resetLevel();
  log3_->resetLevel();
  log4_->resetLevel();

  // Verify that levels are inherited from groups
  EXPECT_TRUE(log1_->level() == Level::TRACE);
  EXPECT_FALSE(log1_->isLevelOverridden());
  EXPECT_TRUE(log2_->level() == Level::TRACE);
  EXPECT_FALSE(log2_->isLevelOverridden());
  EXPECT_TRUE(log3_->level() == Level::TRACE);
  EXPECT_FALSE(log3_->isLevelOverridden());
  EXPECT_TRUE(log4_->level() == Level::TRACE);
  EXPECT_FALSE(log4_->isLevelOverridden());
}

/**
 * @test ChangeSink
 * @brief Tests changing and resetting sinks in loggers.
 *
 * @given A logging system with a hierarchical group structure.
 * @when A new sink is explicitly set for some loggers.
 * @then The new sink is applied and marked as overridden.
 * @when The sinks are reset.
 * @then The sink is inherited from the group and marked as not overridden.
 */
TEST_F(LoggerTest, ChangeSink) {
  // Set new sinks for loggers
  log1_->setSink(sink2_);
  log2_->setSink(sink2_);
  log3_->setSink(sink2_);
  log4_->setSink(sink2_);

  // Verify that sinks are set and marked as overridden
  EXPECT_TRUE(log1_->sink() == sink2_);
  EXPECT_TRUE(log1_->isSinkOverridden());
  EXPECT_TRUE(log2_->sink() == sink2_);
  EXPECT_TRUE(log2_->isSinkOverridden());
  EXPECT_TRUE(log3_->sink() == sink2_);
  EXPECT_TRUE(log3_->isSinkOverridden());
  EXPECT_TRUE(log4_->sink() == sink2_);
  EXPECT_TRUE(log4_->isSinkOverridden());

  // Reset sinks back to group values
  log1_->resetSink();
  log2_->resetSink();
  log3_->resetSink();
  log4_->resetSink();

  // Verify that sinks are inherited from groups
  EXPECT_TRUE(log1_->sink() == sink1_);
  EXPECT_FALSE(log1_->isSinkOverridden());
  EXPECT_TRUE(log2_->sink() == sink1_);
  EXPECT_FALSE(log2_->isSinkOverridden());
  EXPECT_TRUE(log3_->sink() == sink1_);
  EXPECT_FALSE(log3_->isSinkOverridden());
  EXPECT_TRUE(log4_->sink() == sink1_);
  EXPECT_FALSE(log4_->isSinkOverridden());
}
