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

class GroupTest : public ::testing::Test {
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
      system.makeGroup("second", "first", {}, {});
      system.makeGroup("third", "second", "sink3", Level::DEBUG);
      system.makeGroup("four", {}, "sink4", Level::VERBOSE);
      return Configurator::Result{};
    }));
    EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

    EXPECT_NO_THROW(auto r = system_->configure());

    group1_ = system_->getGroup("first");
    ASSERT_TRUE(group1_ != nullptr);
    group2_ = system_->getGroup("second");
    ASSERT_TRUE(group2_ != nullptr);
    group3_ = system_->getGroup("third");
    ASSERT_TRUE(group3_ != nullptr);
    group4_ = system_->getGroup("four");
    ASSERT_TRUE(group4_ != nullptr);
    sink1_ = system_->getSink("sink1");
    ASSERT_TRUE(sink1_ != nullptr);
    sink2_ = system_->getSink("sink2");
    ASSERT_TRUE(sink2_ != nullptr);
    sink3_ = system_->getSink("sink3");
    ASSERT_TRUE(sink3_ != nullptr);
    sink4_ = system_->getSink("sink4");
    ASSERT_TRUE(sink4_ != nullptr);
  }
  void TearDown() override {}

 protected:
  std::shared_ptr<ConfiguratorMock> configurator_;
  std::shared_ptr<LoggingSystem> system_;
  std::shared_ptr<Group> group1_;
  std::shared_ptr<Group> group2_;
  std::shared_ptr<Group> group3_;
  std::shared_ptr<Group> group4_;
  std::shared_ptr<Sink> sink1_;
  std::shared_ptr<Sink> sink2_;
  std::shared_ptr<Sink> sink3_;
  std::shared_ptr<Sink> sink4_;
};

TEST_F(GroupTest, MakeGroup) {
  /// @Given initial state for all next tests

  // If parent isn't set, properties must be provided and not mark as overridden
  EXPECT_TRUE(group1_->parent() == nullptr);
  EXPECT_TRUE(group1_->level() == Level::TRACE);
  EXPECT_FALSE(group1_->isLevelOverridden());
  EXPECT_TRUE(group1_->sink() == sink1_);
  EXPECT_FALSE(group1_->isSinkOverridden());

  // If parent is set and properties aren't provided, then they are inherited
  EXPECT_TRUE(group2_->parent() == group1_);
  EXPECT_TRUE(group2_->level() == Level::TRACE);
  EXPECT_FALSE(group2_->isLevelOverridden());
  EXPECT_TRUE(group2_->sink() == sink1_);
  EXPECT_FALSE(group2_->isSinkOverridden());

  // If parent is set and properties are provided, then they mark as overridden
  EXPECT_TRUE(group3_->parent() == group2_);
  EXPECT_TRUE(group3_->level() == Level::DEBUG);
  EXPECT_TRUE(group3_->isLevelOverridden());
  EXPECT_TRUE(group3_->sink() == sink3_);
  EXPECT_TRUE(group3_->isSinkOverridden());
}

TEST_F(GroupTest, ChangeLevel) {
  /// @When set level

  group2_->setLevel(Level::CRITICAL);
  group3_->setLevel(Level::INFO);

  /// @Then level is set and marked as overridden

  EXPECT_TRUE(group2_->level() == Level::CRITICAL);
  EXPECT_TRUE(group2_->isLevelOverridden());
  EXPECT_TRUE(group3_->level() == Level::INFO);
  EXPECT_TRUE(group3_->isLevelOverridden());

  /// @When reset level

  group2_->resetLevel();
  group3_->resetLevel();

  /// @Then level is set from parent group and marked as no overridden

  EXPECT_TRUE(group2_->level() == Level::TRACE);
  EXPECT_FALSE(group2_->isLevelOverridden());
  EXPECT_TRUE(group3_->level() == Level::TRACE);
  EXPECT_FALSE(group3_->isLevelOverridden());
}

TEST_F(GroupTest, ChangeSink) {
  /// @When set sink

  group2_->setSink(sink3_);
  group3_->setSink(sink4_);

  /// @Then sink is set to provided and marked as overridden

  EXPECT_TRUE(group2_->sink() == sink3_);
  EXPECT_TRUE(group2_->isSinkOverridden());
  EXPECT_TRUE(group3_->sink() == sink4_);
  EXPECT_TRUE(group3_->isSinkOverridden());

  /// @When reset sink

  group2_->resetSink();
  group3_->resetSink();

  /// @Then sink is set from parent group and marked as no overridden

  EXPECT_TRUE(group2_->sink() == sink1_);
  EXPECT_FALSE(group2_->isSinkOverridden());
  EXPECT_TRUE(group3_->sink() == sink1_);
  EXPECT_FALSE(group3_->isSinkOverridden());
}

TEST_F(GroupTest, ChangeGroup) {
  /// @When set other parent group

  group2_->setParentGroup(group4_);
  group3_->setParentGroup(group4_);

  /// @Then parent is changed and
  ///  no overridden properties are set from new parent group

  EXPECT_TRUE(group2_->parent() == group4_);
  EXPECT_TRUE(group2_->level() == Level::VERBOSE);
  EXPECT_FALSE(group2_->isLevelOverridden());
  EXPECT_TRUE(group2_->sink() == sink4_);
  EXPECT_FALSE(group2_->isSinkOverridden());

  EXPECT_TRUE(group3_->parent() == group4_);
  EXPECT_TRUE(group3_->level() == Level::DEBUG);
  EXPECT_TRUE(group3_->isLevelOverridden());
  EXPECT_TRUE(group3_->sink() == sink3_);
  EXPECT_TRUE(group3_->isSinkOverridden());

  /// @When unset parent group

  group2_->unsetParentGroup();
  group3_->unsetParentGroup();

  /// @Then parent group changes to nullptr

  EXPECT_TRUE(group2_->parent() == nullptr);
  EXPECT_TRUE(group2_->level() == Level::VERBOSE);
  EXPECT_FALSE(group2_->isLevelOverridden());
  EXPECT_TRUE(group2_->sink() == sink4_);
  EXPECT_FALSE(group2_->isSinkOverridden());

  EXPECT_TRUE(group3_->parent() == nullptr);
  EXPECT_TRUE(group3_->level() == Level::DEBUG);
  EXPECT_TRUE(group3_->isLevelOverridden());
  EXPECT_TRUE(group3_->sink() == sink3_);
  EXPECT_TRUE(group3_->isSinkOverridden());
}
