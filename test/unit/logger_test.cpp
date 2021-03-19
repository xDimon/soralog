/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

    group1_ = system_->getGroup("first");
    ASSERT_TRUE(group1_ != nullptr);
    group2_ = system_->getGroup("second");
    ASSERT_TRUE(group2_ != nullptr);

    sink1_ = system_->getSink("sink1");
    ASSERT_TRUE(sink1_ != nullptr);
    sink2_ = system_->getSink("sink2");
    ASSERT_TRUE(sink2_ != nullptr);
    sink3_ = system_->getSink("sink3");
    ASSERT_TRUE(sink3_ != nullptr);
    sink4_ = system_->getSink("sink4");
    ASSERT_TRUE(sink4_ != nullptr);

    log1_ = system_->getLogger("log1", "first");
    ASSERT_TRUE(log1_ != nullptr);
    log2_ = system_->getLogger("log2", "first", "sink3");
    ASSERT_TRUE(log2_ != nullptr);
    log3_ = system_->getLogger("log3", "first", Level::INFO);
    ASSERT_TRUE(log3_ != nullptr);
    log4_ = system_->getLogger("log4", "first", "sink4", Level::VERBOSE);
    ASSERT_TRUE(log4_ != nullptr);
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

TEST_F(LoggerTest, MakeLogger) {
  /// @Given initial state for all next tests

  EXPECT_TRUE(log1_->group() == group1_);
  EXPECT_TRUE(log1_->level() == Level::TRACE);
  EXPECT_FALSE(log1_->isLevelOverridden());
  EXPECT_TRUE(log1_->sink() == sink1_);
  EXPECT_FALSE(log1_->isSinkOverridden());

  EXPECT_TRUE(log2_->group() == group1_);
  EXPECT_TRUE(log2_->level() == Level::TRACE);
  EXPECT_FALSE(log2_->isLevelOverridden());
  EXPECT_TRUE(log2_->sink() == sink3_);
  EXPECT_TRUE(log2_->isSinkOverridden());

  EXPECT_TRUE(log3_->group() == group1_);
  EXPECT_TRUE(log3_->level() == Level::INFO);
  EXPECT_TRUE(log3_->isLevelOverridden());
  EXPECT_TRUE(log3_->sink() == sink1_);
  EXPECT_FALSE(log3_->isSinkOverridden());

  EXPECT_TRUE(log4_->group() == group1_);
  EXPECT_TRUE(log4_->level() == Level::VERBOSE);
  EXPECT_TRUE(log4_->isLevelOverridden());
  EXPECT_TRUE(log4_->sink() == sink4_);
  EXPECT_TRUE(log4_->isSinkOverridden());
}

TEST_F(LoggerTest, ChangeLevel) {
  /// @When set level

  log1_->setLevel(Level::CRITICAL);
  log2_->setLevel(Level::CRITICAL);
  log3_->setLevel(Level::CRITICAL);
  log4_->setLevel(Level::CRITICAL);

  /// @Then level is set and marked as overridden

  EXPECT_TRUE(log1_->level() == Level::CRITICAL);
  EXPECT_TRUE(log1_->isLevelOverridden());
  EXPECT_TRUE(log2_->level() == Level::CRITICAL);
  EXPECT_TRUE(log2_->isLevelOverridden());
  EXPECT_TRUE(log3_->level() == Level::CRITICAL);
  EXPECT_TRUE(log3_->isLevelOverridden());
  EXPECT_TRUE(log4_->level() == Level::CRITICAL);
  EXPECT_TRUE(log4_->isLevelOverridden());

  /// @When reset level

  log1_->resetLevel();
  log2_->resetLevel();
  log3_->resetLevel();
  log4_->resetLevel();

  /// @Then level is set from parent group and marked as no overridden

  EXPECT_TRUE(log1_->level() == Level::TRACE);
  EXPECT_FALSE(log1_->isLevelOverridden());
  EXPECT_TRUE(log2_->level() == Level::TRACE);
  EXPECT_FALSE(log2_->isLevelOverridden());
  EXPECT_TRUE(log3_->level() == Level::TRACE);
  EXPECT_FALSE(log3_->isLevelOverridden());
  EXPECT_TRUE(log4_->level() == Level::TRACE);
  EXPECT_FALSE(log4_->isLevelOverridden());
}

TEST_F(LoggerTest, ChangeSink) {
  /// @When set sink

  log1_->setSink(sink2_);
  log2_->setSink(sink2_);
  log3_->setSink(sink2_);
  log4_->setSink(sink2_);

  /// @Then sink is set to provided and marked as overridden

  EXPECT_TRUE(log1_->sink() == sink2_);
  EXPECT_TRUE(log1_->isSinkOverridden());
  EXPECT_TRUE(log2_->sink() == sink2_);
  EXPECT_TRUE(log2_->isSinkOverridden());
  EXPECT_TRUE(log3_->sink() == sink2_);
  EXPECT_TRUE(log3_->isSinkOverridden());
  EXPECT_TRUE(log4_->sink() == sink2_);
  EXPECT_TRUE(log4_->isSinkOverridden());

  /// @When reset sink

  log1_->resetSink();
  log2_->resetSink();
  log3_->resetSink();
  log4_->resetSink();

  /// @Then sink is set from parent group and marked as no overridden

  EXPECT_TRUE(log1_->sink() == sink1_);
  EXPECT_FALSE(log1_->isSinkOverridden());
  EXPECT_TRUE(log2_->sink() == sink1_);
  EXPECT_FALSE(log2_->isSinkOverridden());
  EXPECT_TRUE(log3_->sink() == sink1_);
  EXPECT_FALSE(log3_->isSinkOverridden());
  EXPECT_TRUE(log4_->sink() == sink1_);
  EXPECT_FALSE(log4_->isSinkOverridden());
}

TEST_F(LoggerTest, ChangeGroup) {
  /// @When set other parent group

  log1_->setGroup(group2_);
  log2_->setGroup(group2_);
  log3_->setGroup(group2_);
  log4_->setGroup(group2_);

  /// @Then parent is changed and
  ///  no overridden properties are set from new parent group

  EXPECT_TRUE(log1_->group() == group2_);
  EXPECT_TRUE(log1_->level() == Level::DEBUG);
  EXPECT_FALSE(log1_->isLevelOverridden());
  EXPECT_TRUE(log1_->sink() == sink2_);
  EXPECT_FALSE(log1_->isSinkOverridden());

  EXPECT_TRUE(log2_->group() == group2_);
  EXPECT_TRUE(log2_->level() == Level::DEBUG);
  EXPECT_FALSE(log2_->isLevelOverridden());
  EXPECT_TRUE(log2_->sink() == sink3_);
  EXPECT_TRUE(log2_->isSinkOverridden());

  EXPECT_TRUE(log3_->group() == group2_);
  EXPECT_TRUE(log3_->level() == Level::INFO);
  EXPECT_TRUE(log3_->isLevelOverridden());
  EXPECT_TRUE(log3_->sink() == sink2_);
  EXPECT_FALSE(log3_->isSinkOverridden());

  EXPECT_TRUE(log4_->group() == group2_);
  EXPECT_TRUE(log4_->level() == Level::VERBOSE);
  EXPECT_TRUE(log4_->isLevelOverridden());
  EXPECT_TRUE(log4_->sink() == sink4_);
  EXPECT_TRUE(log4_->isSinkOverridden());
}
