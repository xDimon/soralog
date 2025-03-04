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
 * @class GroupTest
 * @brief Test fixture for testing Group functionality in LoggingSystem.
 *
 * This fixture initializes a logging system with multiple groups and sinks
 * for testing various configurations and modifications of logging groups.
 */
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

    ASSERT_TRUE(group1_ = system_->getGroup("first"));
    ASSERT_TRUE(group2_ = system_->getGroup("second"));
    ASSERT_TRUE(group3_ = system_->getGroup("third"));
    ASSERT_TRUE(group4_ = system_->getGroup("four"));
    ASSERT_TRUE(sink1_ = system_->getSink("sink1"));
    ASSERT_TRUE(sink2_ = system_->getSink("sink2"));
    ASSERT_TRUE(sink3_ = system_->getSink("sink3"));
    ASSERT_TRUE(sink4_ = system_->getSink("sink4"));
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

/**
 * @test MakeGroup
 * @brief Tests group creation and property inheritance.
 *
 * @given A logging system with multiple groups and sinks.
 * @when Groups are created with or without explicitly set properties.
 * @then Groups inherit properties from their parent unless explicitly
 * overridden.
 */
TEST_F(GroupTest, MakeGroup) {
  // If parent isn't set, properties must be provided and not marked as
  // overridden
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

  // If parent is set and properties are provided, then they are marked as
  // overridden
  EXPECT_TRUE(group3_->parent() == group2_);
  EXPECT_TRUE(group3_->level() == Level::DEBUG);
  EXPECT_TRUE(group3_->isLevelOverridden());
  EXPECT_TRUE(group3_->sink() == sink3_);
  EXPECT_TRUE(group3_->isSinkOverridden());
}

/**
 * @test ChangeLevel
 * @brief Tests changing and resetting logging levels in groups.
 *
 * @given A logging system with a hierarchical group structure.
 * @when Levels are explicitly set for some groups.
 * @then The new level is applied and marked as overridden.
 * @when The levels are reset.
 * @then The level is inherited from the parent and marked as not overridden.
 */
TEST_F(GroupTest, ChangeLevel) {
  // Set custom logging levels for groups
  group2_->setLevel(Level::CRITICAL);
  group3_->setLevel(Level::INFO);

  // Verify that the levels are set and marked as overridden
  EXPECT_TRUE(group2_->level() == Level::CRITICAL);
  EXPECT_TRUE(group2_->isLevelOverridden());
  EXPECT_TRUE(group3_->level() == Level::INFO);
  EXPECT_TRUE(group3_->isLevelOverridden());

  // Reset levels back to parent values
  group2_->resetLevel();
  group3_->resetLevel();

  // Verify that levels are inherited from parent groups
  EXPECT_TRUE(group2_->level() == Level::TRACE);
  EXPECT_FALSE(group2_->isLevelOverridden());
  EXPECT_TRUE(group3_->level() == Level::TRACE);
  EXPECT_FALSE(group3_->isLevelOverridden());
}

/**
 * @test ChangeSink
 * @brief Tests changing and resetting sinks in groups.
 *
 * @given A logging system with a hierarchical group structure.
 * @when A new sink is explicitly set for some groups.
 * @then The new sink is applied and marked as overridden.
 * @when The sinks are reset.
 * @then The sink is inherited from the parent and marked as not overridden.
 */
TEST_F(GroupTest, ChangeSink) {
  // Set new sinks for groups
  group2_->setSink(sink3_);
  group3_->setSink(sink4_);

  // Verify that sinks are set and marked as overridden
  EXPECT_TRUE(group2_->sink() == sink3_);
  EXPECT_TRUE(group2_->isSinkOverridden());
  EXPECT_TRUE(group3_->sink() == sink4_);
  EXPECT_TRUE(group3_->isSinkOverridden());

  // Reset sinks back to parent values
  group2_->resetSink();
  group3_->resetSink();

  // Verify that sinks are inherited from parent groups
  EXPECT_TRUE(group2_->sink() == sink1_);
  EXPECT_FALSE(group2_->isSinkOverridden());
  EXPECT_TRUE(group3_->sink() == sink1_);
  EXPECT_FALSE(group3_->isSinkOverridden());
}

/**
 * @test ChangeGroup
 * @brief Tests changing and unsetting parent groups.
 *
 * @given A logging system with a hierarchical group structure.
 * @when The parent of a group is changed to another group.
 * @then The group inherits properties from the new parent unless explicitly
 * overridden.
 * @when The parent group is unset.
 * @then The group becomes independent and retains its last known properties.
 */
TEST_F(GroupTest, ChangeGroup) {
  // Change the parent group of two groups
  group2_->setParentGroup(group4_);
  group3_->setParentGroup(group4_);

  // Verify that the parent group is changed
  // and inherited properties are updated
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

  // Unset the parent group for both groups
  group2_->unsetParentGroup();
  group3_->unsetParentGroup();

  // Verify that the groups no longer have a parent
  // but retain their last known properties
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
