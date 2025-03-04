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
 * @class LoggingSystemTest
 * @brief Unit tests for the LoggingSystem class using Google Test framework.
 *
 * This test fixture provides a setup for testing the LoggingSystem, allowing
 * the creation of mock sinks and groups. The configurator mock is used to
 * simulate configuration behavior.
 */
class LoggingSystemTest : public ::testing::Test {
 public:
  /**
   * @brief Sets up the test environment before each test case.
   *
   * Initializes the configurator mock and creates an instance of the
   * LoggingSystem.
   */
  void SetUp() override {
    configurator_ = std::make_shared<ConfiguratorMock>();
    system_ = std::make_shared<LoggingSystem>(configurator_);
  }

  /**
   * @brief Cleans up resources after each test case.
   *
   * Currently does nothing but is provided for future cleanup needs.
   */
  void TearDown() override {}

  /**
   * @brief Configures the LoggingSystem with predefined mock sinks and groups.
   *
   * This method simulates applying a configuration by:
   * - Creating two mock sinks (`sink` and `other`).
   * - Setting expectations for the `flush` method on each sink.
   * - Creating three groups with different levels and inheritance settings.
   * - Ensuring the configurator is applied exactly once.
   * - Catching any unexpected exceptions during configuration.
   */
  void configure() {
    ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
      return &s == system_.get();
    }))).WillByDefault(Invoke([&](LoggingSystem &system) {
      // Creating the first sink and setting expectations on flush calls
      auto sink1 = system.makeSink<SinkMock>("sink");
      EXPECT_CALL(*sink1, mocked_flush()).Times(testing::AnyNumber());

      // Creating the second sink and setting expectations on flush calls
      auto sink2 = system.makeSink<SinkMock>("other");
      EXPECT_CALL(*sink2, mocked_flush()).Times(testing::AnyNumber());

      // Creating groups with hierarchical relationships
      system.makeGroup("first", {}, "sink", Level::VERBOSE);
      system.makeGroup("second", "first", "sink", Level::DEBUG);
      system.makeGroup("third", "second", "sink", Level::TRACE);

      return Configurator::Result{};
    }));

    // Expect the `applyOn` method to be called exactly once
    EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

    // Ensure configuration does not throw any exceptions
    EXPECT_NO_THROW(auto r = system_->configure());
  }

 protected:
  /// Mock configurator instance
  std::shared_ptr<ConfiguratorMock> configurator_;
  /// Logging system instance
  std::shared_ptr<LoggingSystem> system_;
};

/**
 * @brief Tests configuration of the LoggingSystem.
 *
 * @given An unconfigured LoggingSystem.
 * @when The configure method is called.
 * @then The system is successfully configured, and a second configuration
 *       attempt throws a logic error.
 */
TEST_F(LoggingSystemTest, Configure) {
  EXPECT_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillRepeatedly(Return(Configurator::Result{}));

  // Verify that the default sink exists before configuration
  EXPECT_TRUE(system_->getSink("*") != nullptr);

  // First configuration should succeed
  EXPECT_NO_THROW(auto r = system_->configure());

  // Second configuration should fail with a logic error
  EXPECT_THROW(auto r = system_->configure(), std::logic_error);
}

/**
 * @brief Tests creation of groups in the LoggingSystem.
 *
 * @given An empty LoggingSystem.
 * @when Groups are created with various parameters.
 * @then A valid group is created when parameters are correct,
 *       and errors are thrown for invalid parameters.
 */
TEST_F(LoggingSystemTest, MakeGroup) {
  // Initially, there should be no fallback group
  EXPECT_TRUE(system_->getFallbackGroup() == nullptr);

  // Attempting to create a group without specifying a level should fail
  EXPECT_ANY_THROW(system_->makeGroup("first", {}, {}, {}));

  // Creating a group with a valid level should succeed
  EXPECT_NO_THROW(system_->makeGroup("first", {}, {}, Level::INFO));

  // The first created group should become the fallback group
  auto defGroup = system_->getFallbackGroup();
  auto firstGroup = system_->getGroup("first");
  ASSERT_TRUE(defGroup != nullptr);
  EXPECT_TRUE(firstGroup == defGroup);

  // Creating a group with a non-existing parent should fail
  EXPECT_ANY_THROW(system_->makeGroup("second", "nonexisting_group", {}, {}));

  // Creating a group with a non-existing sink should fail
  EXPECT_ANY_THROW(system_->makeGroup("second", {}, "nonexisting_sink", {}));

  // Creating a valid group with an existing parent should succeed
  EXPECT_NO_THROW(system_->makeGroup("second", "first", {}, {}));
}

/**
 * @brief Tests creation of sinks in the LoggingSystem.
 *
 * @given An empty LoggingSystem.
 * @when A sink is created.
 * @then The sink is successfully stored in the system.
 */
TEST_F(LoggingSystemTest, MakeSink) {
  // Initially, the sink should not exist
  EXPECT_FALSE(system_->getSink("sink") != nullptr);

  // Creating a new sink should not throw
  EXPECT_NO_THROW(system_->makeSink<SinkMock>("sink"));

  // The sink should now be retrievable
  EXPECT_TRUE(system_->getSink("sink") != nullptr);
}

/**
 * @brief Tests retrieval of groups from the LoggingSystem.
 *
 * @given A configured LoggingSystem with predefined groups.
 * @when Groups are retrieved using their names.
 * @then Existing groups are successfully retrieved, while non-existing groups
 * return nullptr.
 */
TEST_F(LoggingSystemTest, GetGroup) {
  configure();

  // The fallback group should exist
  EXPECT_TRUE(system_->getFallbackGroup() != nullptr);

  // Existing groups should be retrievable
  auto firstGroup = system_->getGroup("first");
  EXPECT_TRUE(firstGroup != nullptr);

  auto secondGroup = system_->getGroup("second");
  EXPECT_TRUE(secondGroup != nullptr);

  auto thirdGroup = system_->getGroup("third");
  EXPECT_TRUE(thirdGroup != nullptr);

  // A non-existing group should return nullptr
  auto fourthGroup = system_->getGroup("fourth");
  EXPECT_FALSE(fourthGroup != nullptr);
}

/**
 * @brief Tests retrieval of sinks from the LoggingSystem.
 *
 * @given A LoggingSystem with and without configuration.
 * @when Sinks are retrieved before and after configuration.
 * @then The default sink is always present, and additional sinks are available
 * after configuration.
 */
TEST_F(LoggingSystemTest, GetSink) {
  // The default sink should always be present
  EXPECT_TRUE(system_->getSink("*") != nullptr);

  // A non-existing sink should not be found
  EXPECT_FALSE(system_->getSink("sink") != nullptr);

  configure();

  // After configuration, the "sink" should exist
  EXPECT_TRUE(system_->getSink("sink") != nullptr);
}

/**
 * @brief Tests retrieval of loggers from the LoggingSystem.
 *
 * @given A configured LoggingSystem with predefined groups and sinks.
 * @when Loggers are requested with different configurations.
 * @then The correct logger is returned with the expected group, sink, and
 * level.
 */
TEST_F(LoggingSystemTest, GetLogger) {
  configure();

  /// @When requesting a logger with a non-existing group
  /// @Then it should be assigned to the fallback group
  {
    auto log0 = system_->getLogger("Log_0", "nonexisting_group");
    ASSERT_TRUE(log0 != nullptr);
    auto group = system_->getFallbackGroup();
    ASSERT_TRUE(group != nullptr);
    EXPECT_TRUE(log0->group() == group);
    EXPECT_TRUE(log0->sink() == group->sink());
    EXPECT_TRUE(log0->level() == group->level());
  }

  /// @When requesting loggers for existing groups
  /// @Then they should be associated with the correct group, sink, and level
  {
    auto log1 = system_->getLogger("Log_1", "first");
    ASSERT_TRUE(log1 != nullptr);
    EXPECT_TRUE(log1->group() == system_->getGroup("first"));
    EXPECT_TRUE(log1->sink() == system_->getSink("sink"));
    EXPECT_TRUE(log1->level() == Level::VERBOSE)
        << "Actual level is '" << levelToStr(log1->level()) << "'";
  }
  {
    auto log2 = system_->getLogger("Log_2", "second");
    ASSERT_TRUE(log2 != nullptr);
    EXPECT_TRUE(log2->group() == system_->getGroup("second"));
    EXPECT_TRUE(log2->sink() == system_->getSink("sink"));
    EXPECT_TRUE(log2->level() == Level::DEBUG)
        << "Actual level is '" << levelToStr(log2->level()) << "'";
  }
  {
    auto log3 = system_->getLogger("Log_3", "third");
    ASSERT_TRUE(log3 != nullptr);
    EXPECT_TRUE(log3->group() == system_->getGroup("third"));
    EXPECT_TRUE(log3->sink() == system_->getSink("sink"));
    EXPECT_TRUE(log3->level() == Level::TRACE)
        << "Actual level is '" << levelToStr(log3->level()) << "'";
  }

  /// @When requesting a logger with a custom sink
  /// @Then the logger should use the specified sink instead of the group sink
  {
    auto log4 = system_->getLogger("Log_4", "first", "other");
    ASSERT_TRUE(log4 != nullptr);
    auto group = log4->group();
    ASSERT_TRUE(group != nullptr);
    auto sink = log4->sink();
    ASSERT_TRUE(sink != nullptr);
    auto level = log4->level();
    ASSERT_TRUE(group == system_->getGroup("first"));
    EXPECT_TRUE(sink != group->sink());
    EXPECT_TRUE(sink == system_->getSink("other"));
    EXPECT_TRUE(level == group->level());
  }

  /// @When requesting a logger with a custom level
  /// @Then the logger should use the specified level instead of the group level
  {
    auto log5 = system_->getLogger("Log_5", "first", Level::INFO);
    ASSERT_TRUE(log5 != nullptr);
    auto group = log5->group();
    ASSERT_TRUE(group != nullptr);
    auto sink = log5->sink();
    ASSERT_TRUE(sink != nullptr);
    auto level = log5->level();
    ASSERT_TRUE(group == system_->getGroup("first"));
    EXPECT_TRUE(sink == group->sink());
    EXPECT_TRUE(level != group->level());
    EXPECT_TRUE(level == Level::INFO);
  }

  /// @When requesting a logger with both a custom sink and a custom level
  /// @Then the logger should use both the specified sink and level
  {
    auto log6 = system_->getLogger("Log_6", "first", "other", Level::INFO);
    ASSERT_TRUE(log6 != nullptr);
    auto group = log6->group();
    ASSERT_TRUE(group != nullptr);
    auto sink = log6->sink();
    ASSERT_TRUE(sink != nullptr);
    auto level = log6->level();
    ASSERT_TRUE(group == system_->getGroup("first"));
    EXPECT_TRUE(sink != group->sink());
    EXPECT_TRUE(sink == system_->getSink("other"));
    EXPECT_TRUE(level != group->level());
    EXPECT_TRUE(level == Level::INFO);
  }
}

/**
 * @brief Tests the behavior of the fallback group in the LoggingSystem.
 *
 * @given An unconfigured LoggingSystem.
 * @when The system is configured and groups are created.
 * @then The first created group is set as the fallback group, and it can be
 * changed.
 */
TEST_F(LoggingSystemTest, FallbackGroup) {
  /// @When the system is not yet configured
  /// @Then there should be no fallback group
  EXPECT_TRUE(system_->getFallbackGroup() == nullptr);

  configure();

  /// @When retrieving groups after configuration
  /// @Then the groups should exist
  auto group1 = system_->getGroup("first");
  EXPECT_TRUE(group1 != nullptr);

  auto group2 = system_->getGroup("second");
  EXPECT_TRUE(group2 != nullptr);

  /// @Then the first created group should be the fallback group
  EXPECT_TRUE(system_->getFallbackGroup() == group1);

  /// @When changing the fallback group to another existing group
  /// @Then the fallback group should be updated correctly
  system_->setFallbackGroup("second");
  EXPECT_TRUE(system_->getFallbackGroup() == group2);
}

/**
 * @brief Tests changing the logging level of different groups within the
 * LoggingSystem.
 *
 * @given A configured logging system with three hierarchical groups and
 * associated loggers.
 * @when The logging level of a group is changed.
 * @then The new level propagates correctly to dependent groups and loggers
 * unless explicitly overridden.
 */
TEST_F(LoggingSystemTest, ChangeLevelOfGroup) {
  /// @Given a configured system with hierarchical groups and loggers
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink");
    system.makeGroup("first", {}, "sink", Level::INFO);
    system.makeGroup("second", "first", {}, {});
    system.makeGroup("third", "second", {}, Level::WARN);
    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  auto group1 = system_->getGroup("first");
  ASSERT_TRUE(group1 != nullptr);
  auto group2 = system_->getGroup("second");
  ASSERT_TRUE(group2 != nullptr);
  auto group3 = system_->getGroup("third");
  ASSERT_TRUE(group3 != nullptr);

  // Create loggers
  auto log1 = system_->getLogger("Log1", "first");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "first", Level::TRACE);
  ASSERT_TRUE(log2 != nullptr);
  auto log3 = system_->getLogger("Log3", "second");
  ASSERT_TRUE(log3 != nullptr);
  auto log4 = system_->getLogger("Log4", "second", Level::DEBUG);
  ASSERT_TRUE(log4 != nullptr);
  auto log5 = system_->getLogger("Log5", "third");
  ASSERT_TRUE(log5 != nullptr);
  auto log6 = system_->getLogger("Log6", "third", Level::VERBOSE);
  ASSERT_TRUE(log6 != nullptr);

  auto expect_initial_state = [&] {
    EXPECT_TRUE(group1->level() == Level::INFO);
    EXPECT_FALSE(group1->isLevelOverridden());

    EXPECT_TRUE(group2->level() == group1->level());
    EXPECT_FALSE(group2->isLevelOverridden());

    EXPECT_TRUE(group3->level() == Level::WARN);
    EXPECT_TRUE(group3->isLevelOverridden());

    EXPECT_TRUE(log1->level() == Level::INFO);
    EXPECT_TRUE(log1->level() == group2->level());
    EXPECT_FALSE(log1->isLevelOverridden());

    EXPECT_TRUE(log2->level() == Level::TRACE);
    EXPECT_TRUE(log2->level() != group1->level());
    EXPECT_TRUE(log2->isLevelOverridden());

    EXPECT_TRUE(log3->level() == Level::INFO);
    EXPECT_TRUE(log3->level() == group2->level());
    EXPECT_FALSE(log3->isLevelOverridden());

    EXPECT_TRUE(log4->level() == Level::DEBUG);
    EXPECT_TRUE(log4->level() != group2->level());
    EXPECT_TRUE(log4->isLevelOverridden());

    EXPECT_TRUE(log5->level() == Level::WARN);
    EXPECT_TRUE(log5->level() == group3->level());
    EXPECT_FALSE(log5->isLevelOverridden());

    EXPECT_TRUE(log6->level() == Level::VERBOSE);
    EXPECT_TRUE(log6->level() != group3->level());
    EXPECT_TRUE(log6->isLevelOverridden());
  };

  /// @Given initial state of groups and loggers
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   I     @1=I  W
  //
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Level:   @1=I  T     @2=I  D     @3=W  V

  expect_initial_state();

  /// @When changing the level of the top-level group
  system_->setLevelOfGroup("first", Level::CRITICAL);

  /// @Then the level change should propagate to dependent groups unless
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   C     @1=C  W
  //           ^        ^
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Level:   @1=C  T     @2=C  D     @3=W  V
  //              ^           ^

  // Properties not overridden because group doesn't have parent
  EXPECT_TRUE(group1->level() == Level::CRITICAL);  // *own
  EXPECT_FALSE(group1->isLevelOverridden());
  EXPECT_TRUE(group2->level() == Level::CRITICAL);  // *g1
  EXPECT_FALSE(group2->isLevelOverridden());  // no overridden because no parent
  EXPECT_TRUE(group3->level() == Level::WARN);  // own
  EXPECT_TRUE(group3->isLevelOverridden());

  EXPECT_TRUE(log1->level() == Level::CRITICAL);  // *g1
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::TRACE);  // own
  EXPECT_TRUE(log2->isLevelOverridden());
  EXPECT_TRUE(log3->level() == Level::CRITICAL);  // *g2<-g1
  EXPECT_FALSE(log3->isLevelOverridden());
  EXPECT_TRUE(log4->level() == Level::DEBUG);  // own
  EXPECT_TRUE(log4->isLevelOverridden());
  EXPECT_TRUE(log5->level() == Level::WARN);  // g3
  EXPECT_FALSE(log5->isLevelOverridden());
  EXPECT_TRUE(log6->level() == Level::VERBOSE);  // own
  EXPECT_TRUE(log6->isLevelOverridden());

  /// @When reverting the level of the top-level group
  system_->setLevelOfGroup("first", Level::INFO);

  /// @Then the initial state should be restored
  expect_initial_state();

  /// @When changing the level of a dependent group
  system_->setLevelOfGroup("second", Level::CRITICAL);

  /// @Then the level change should be applied only
  ///       to the specific group and its inheritors
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   I     C     W
  //                 ^
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Level:   @1=I  T     C     D     @3=W  V
  //                       ^

  EXPECT_TRUE(group1->level() == Level::INFO);  // own
  EXPECT_FALSE(group1->isLevelOverridden());
  EXPECT_TRUE(group2->level() == Level::CRITICAL);  // *own
  EXPECT_TRUE(group2->isLevelOverridden());
  EXPECT_TRUE(group3->level() == Level::WARN);  // own
  EXPECT_TRUE(group3->isLevelOverridden());

  EXPECT_TRUE(log1->level() == Level::INFO);  // g1
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::TRACE);  // own
  EXPECT_TRUE(log2->isLevelOverridden());
  EXPECT_TRUE(log3->level() == Level::CRITICAL);  // *g2
  EXPECT_FALSE(log3->isLevelOverridden());
  EXPECT_TRUE(log4->level() == Level::DEBUG);  // own
  EXPECT_TRUE(log4->isLevelOverridden());
  EXPECT_TRUE(log5->level() == Level::WARN);  // g3
  EXPECT_FALSE(log5->isLevelOverridden());
  EXPECT_TRUE(log6->level() == Level::VERBOSE);  // own
  EXPECT_TRUE(log6->isLevelOverridden());

  /// @When resetting the level of the dependent group
  system_->resetLevelOfGroup("second");

  /// @Then the initial state should be restored again
  expect_initial_state();
}

/**
 * @brief Tests changing the sink of different groups within the LoggingSystem.
 *
 * @given A configured logging system with hierarchical groups and multiple
 * sinks.
 * @when The sink of a group is changed.
 * @then The new sink propagates correctly to dependent groups and loggers
 * unless explicitly overridden.
 */
TEST_F(LoggingSystemTest, ChangeSinkOfGroup) {
  /// @Given a configured system with hierarchical groups and multiple sinks
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink1");
    system.makeSink<SinkMock>("sink2");
    system.makeSink<SinkMock>("sink3");
    system.makeSink<SinkMock>("sink4");
    system.makeSink<SinkMock>("sink5");
    system.makeSink<SinkMock>("sink6");
    system.makeGroup("first", {}, "sink1", Level::INFO);
    system.makeGroup("second", "first", {}, {});
    system.makeGroup("third", "second", "sink2", {});
    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  auto sink1 = system_->getSink("sink1");
  ASSERT_TRUE(sink1 != nullptr);
  auto sink2 = system_->getSink("sink2");
  ASSERT_TRUE(sink2 != nullptr);
  auto sink3 = system_->getSink("sink3");
  ASSERT_TRUE(sink3 != nullptr);
  auto sink4 = system_->getSink("sink4");
  ASSERT_TRUE(sink4 != nullptr);
  auto sink5 = system_->getSink("sink5");
  ASSERT_TRUE(sink5 != nullptr);
  auto sink6 = system_->getSink("sink6");
  ASSERT_TRUE(sink6 != nullptr);

  auto group1 = system_->getGroup("first");
  ASSERT_TRUE(group1 != nullptr);
  auto group2 = system_->getGroup("second");
  ASSERT_TRUE(group2 != nullptr);
  auto group3 = system_->getGroup("third");
  ASSERT_TRUE(group3 != nullptr);

  // Create loggers
  auto log1 = system_->getLogger("Log1", "first");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "first", "sink3");
  ASSERT_TRUE(log2 != nullptr);
  auto log3 = system_->getLogger("Log3", "second");
  ASSERT_TRUE(log3 != nullptr);
  auto log4 = system_->getLogger("Log4", "second", "sink4");
  ASSERT_TRUE(log4 != nullptr);
  auto log5 = system_->getLogger("Log5", "third");
  ASSERT_TRUE(log5 != nullptr);
  auto log6 = system_->getLogger("Log6", "third", "sink5");
  ASSERT_TRUE(log6 != nullptr);

  /// Verifies the initial state of groups and loggers.
  auto expect_initial_state = [&] {
    EXPECT_TRUE(group1->sink() == sink1);
    EXPECT_FALSE(group1->isSinkOverridden());

    EXPECT_TRUE(group2->sink() == group1->sink());
    EXPECT_FALSE(group2->isSinkOverridden());

    EXPECT_TRUE(group3->sink() == sink2);
    EXPECT_TRUE(group3->isSinkOverridden());

    EXPECT_TRUE(log1->sink() == sink1);
    EXPECT_TRUE(log1->sink() == group2->sink());
    EXPECT_FALSE(log1->isSinkOverridden());

    EXPECT_TRUE(log2->sink() == sink3);
    EXPECT_TRUE(log2->sink() != group1->sink());
    EXPECT_TRUE(log2->isSinkOverridden());

    EXPECT_TRUE(log3->sink() == sink1);
    EXPECT_TRUE(log3->sink() == group2->sink());
    EXPECT_FALSE(log3->isSinkOverridden());

    EXPECT_TRUE(log4->sink() == sink4);
    EXPECT_TRUE(log4->sink() != group2->sink());
    EXPECT_TRUE(log4->isSinkOverridden());

    EXPECT_TRUE(log5->sink() == sink2);
    EXPECT_TRUE(log5->sink() == group3->sink());
    EXPECT_FALSE(log5->isSinkOverridden());

    EXPECT_TRUE(log6->sink() == sink5);
    EXPECT_TRUE(log6->sink() != group3->sink());
    EXPECT_TRUE(log6->isSinkOverridden());
  };

  /// @Given initial state of groups and loggers
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S1    @1=S1 S2
  //
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Sink:    @1=S1 S3    @2=S1 S4    @3=S2 S5

  expect_initial_state();

  /// @When changing the sink of the top-level group
  system_->setSinkOfGroup("first", "sink6");

  /// @Then the sink change should propagate to dependent groups unless
  ///       overridden
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S6    @1=S6 S2
  //           ^        ^
  //
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Sink:    @1=S6 S3    @2=S6 S4    @3=S2 S5
  //              ^           ^

  EXPECT_TRUE(group1->sink() == sink6);  // *own
  EXPECT_FALSE(group1->isSinkOverridden());
  EXPECT_TRUE(group2->sink() == sink6);  // *g1
  EXPECT_FALSE(group2->isSinkOverridden());
  EXPECT_TRUE(group3->sink() == sink2);  // own
  EXPECT_TRUE(group3->isSinkOverridden());

  EXPECT_TRUE(log1->sink() == sink6);  // *g1
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink3);  // own
  EXPECT_TRUE(log2->isSinkOverridden());
  EXPECT_TRUE(log3->sink() == sink6);  // *g2<-g1
  EXPECT_FALSE(log3->isSinkOverridden());
  EXPECT_TRUE(log4->sink() == sink4);  // own
  EXPECT_TRUE(log4->isSinkOverridden());
  EXPECT_TRUE(log5->sink() == sink2);  // g3
  EXPECT_FALSE(log5->isSinkOverridden());
  EXPECT_TRUE(log6->sink() == sink5);  // own
  EXPECT_TRUE(log6->isSinkOverridden());

  /// @When reverting the sink of the top-level group
  system_->setSinkOfGroup("first", "sink1");

  /// @Then the initial state should be restored
  expect_initial_state();

  /// @When changing the sink of a dependent group
  system_->setSinkOfGroup("second", "sink6");

  /// @Then the sink change should be applied only to the specific group and its
  ///       inheritors
  //
  // State of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S1    S6    S2
  //                 ^
  //
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G3
  //  Sink:    @1=S1 S3    S6    S4    @3=S2 S5
  //                       ^

  EXPECT_TRUE(group1->sink() == sink1);  // own
  EXPECT_FALSE(group1->isSinkOverridden());
  EXPECT_TRUE(group2->sink() == sink6);  // *own
  EXPECT_TRUE(group2->isSinkOverridden());
  EXPECT_TRUE(group3->sink() == sink2);  // own
  EXPECT_TRUE(group3->isSinkOverridden());

  EXPECT_TRUE(log1->sink() == sink1);  // g1
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink3);  // own
  EXPECT_TRUE(log2->isSinkOverridden());
  EXPECT_TRUE(log3->sink() == sink6);  // *g2
  EXPECT_FALSE(log3->isSinkOverridden());
  EXPECT_TRUE(log4->sink() == sink4);  // own
  EXPECT_TRUE(log4->isSinkOverridden());
  EXPECT_TRUE(log5->sink() == sink2);  // g3
  EXPECT_FALSE(log5->isSinkOverridden());
  EXPECT_TRUE(log6->sink() == sink5);  // own
  EXPECT_TRUE(log6->isSinkOverridden());

  /// @When resetting the sink of the dependent group
  system_->resetSinkOfGroup("second");

  /// @Then the initial state should be restored again
  expect_initial_state();
}

/**
 * @brief Tests changing the parent group within the LoggingSystem.
 *
 * @given A configured logging system with hierarchical groups and multiple
 * sinks.
 * @when The parent group of a group is changed.
 * @then The new parent group propagates its settings correctly to dependent
 * groups and loggers unless explicitly overridden.
 */
TEST_F(LoggingSystemTest, ChangeParentGroup) {
  /// @Given a configured system with hierarchical groups and multiple sinks
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink1");
    system.makeSink<SinkMock>("sink2");
    system.makeSink<SinkMock>("sink3");

    system.makeGroup("first1", {}, "sink1", Level::TRACE);
    system.makeGroup("first2", {}, "sink2", Level::DEBUG);
    system.makeGroup("second1", "first1", {}, {});
    system.makeGroup("second2", "first1", {}, {});
    system.makeGroup("third1", "second1", "sink3", {});
    system.makeGroup("third2", "second1", {}, Level::CRITICAL);

    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  /// @Given the initial state of groups and their relationships
  auto sink1 = system_->getSink("sink1");
  ASSERT_TRUE(sink1 != nullptr);
  auto sink2 = system_->getSink("sink2");
  ASSERT_TRUE(sink2 != nullptr);
  auto sink3 = system_->getSink("sink3");
  ASSERT_TRUE(sink3 != nullptr);

  auto group11 = system_->getGroup("first1");
  ASSERT_TRUE(group11 != nullptr);
  auto group21 = system_->getGroup("second1");
  ASSERT_TRUE(group21 != nullptr);
  auto group31 = system_->getGroup("third1");
  ASSERT_TRUE(group31 != nullptr);
  auto group12 = system_->getGroup("first2");
  ASSERT_TRUE(group12 != nullptr);
  auto group22 = system_->getGroup("second2");
  ASSERT_TRUE(group22 != nullptr);
  auto group32 = system_->getGroup("third2");
  ASSERT_TRUE(group32 != nullptr);

  /// @Given loggers attached to different groups
  auto log1 = system_->getLogger("Log1", "first1");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "first2");
  ASSERT_TRUE(log2 != nullptr);
  auto log3 = system_->getLogger("Log3", "second1");
  ASSERT_TRUE(log3 != nullptr);
  auto log4 = system_->getLogger("Log4", "second2");
  ASSERT_TRUE(log4 != nullptr);
  auto log5 = system_->getLogger("Log5", "third1");
  ASSERT_TRUE(log5 != nullptr);
  auto log6 = system_->getLogger("Log6", "third2");
  ASSERT_TRUE(log6 != nullptr);

  // Hierarchy of groups
  //    G11 -- G21 -- G31
  //        \      `- G32
  //         ` G22
  //    G12
  //
  // State of groups
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G11    G11    G21    G21
  //  Sink:    S1     S2     @11=S1 @11=S1 =S3 @21=S1
  //  Level:   T      D      @11=T  G11=T  @21=T  =C
  //
  // State of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G11   G12   G21   G22   G31   G32

  EXPECT_TRUE(group11->parent() == nullptr);
  EXPECT_TRUE(group11->sink() == sink1);
  EXPECT_FALSE(group11->isSinkOverridden());
  EXPECT_TRUE(group11->level() == Level::TRACE);
  EXPECT_FALSE(group11->isLevelOverridden());
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_TRUE(log1->level() == Level::TRACE);

  EXPECT_TRUE(group12->parent() == nullptr);
  EXPECT_TRUE(group12->sink() == sink2);
  EXPECT_FALSE(group12->isSinkOverridden());
  EXPECT_TRUE(group12->level() == Level::DEBUG);
  EXPECT_FALSE(group12->isLevelOverridden());
  EXPECT_TRUE(log2->sink() == sink2);
  EXPECT_TRUE(log2->level() == Level::DEBUG);

  EXPECT_TRUE(group21->parent() == group11);
  EXPECT_TRUE(group21->sink() == sink1);
  EXPECT_FALSE(group21->isSinkOverridden());
  EXPECT_TRUE(group21->level() == Level::TRACE);
  EXPECT_FALSE(group21->isLevelOverridden());
  EXPECT_TRUE(log3->sink() == sink1);
  EXPECT_TRUE(log3->level() == Level::TRACE);

  EXPECT_TRUE(group22->parent() == group11);
  EXPECT_TRUE(group22->sink() == sink1);
  EXPECT_FALSE(group22->isSinkOverridden());
  EXPECT_TRUE(group22->level() == Level::TRACE);
  EXPECT_FALSE(group22->isLevelOverridden());
  EXPECT_TRUE(log4->sink() == sink1);
  EXPECT_TRUE(log4->level() == Level::TRACE);

  EXPECT_TRUE(group31->parent() == group21);
  EXPECT_TRUE(group31->sink() == sink3);
  EXPECT_TRUE(group31->isSinkOverridden());
  EXPECT_TRUE(group31->level() == Level::TRACE);
  EXPECT_FALSE(group31->isLevelOverridden());
  EXPECT_TRUE(log5->sink() == sink3);
  EXPECT_TRUE(log5->level() == Level::TRACE);

  EXPECT_TRUE(group32->parent() == group21);
  EXPECT_TRUE(group32->sink() == sink1);
  EXPECT_FALSE(group32->isSinkOverridden());
  EXPECT_TRUE(group32->level() == Level::CRITICAL);
  EXPECT_TRUE(group32->isLevelOverridden());
  EXPECT_TRUE(log6->sink() == sink1);
  EXPECT_TRUE(log6->level() == Level::CRITICAL);

  /// @When changing the parent of a second-level group
  system_->setParentOfGroup("second1", "first2");

  /// @Then the group should inherit properties from the new parent
  //
  // Hierarchy of groups
  //    G11 -- G22
  //    G12 -- G21 -- G31
  //               `- G32
  //
  // State of groups
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G12    G11    G21    G21
  //  Sink:    S1     S2     @12=S2 @11=S1 =S3 @21=S2
  //  Level:   T      D      @12=D  G11=T  @21=D  =C
  //                  ^                ^      ^

  ///
  EXPECT_TRUE(group11->parent() == nullptr);
  EXPECT_TRUE(group11->sink() == sink1);
  EXPECT_FALSE(group11->isSinkOverridden());
  EXPECT_TRUE(group11->level() == Level::TRACE);
  EXPECT_FALSE(group11->isLevelOverridden());
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_TRUE(log1->level() == Level::TRACE);

  EXPECT_TRUE(group12->parent() == nullptr);
  EXPECT_TRUE(group12->sink() == sink2);
  EXPECT_FALSE(group12->isSinkOverridden());
  EXPECT_TRUE(group12->level() == Level::DEBUG);
  EXPECT_FALSE(group12->isLevelOverridden());
  EXPECT_TRUE(log2->sink() == sink2);
  EXPECT_TRUE(log2->level() == Level::DEBUG);

  EXPECT_TRUE(group21->parent() == group12);
  EXPECT_TRUE(group21->sink() == sink2);
  EXPECT_FALSE(group21->isSinkOverridden());
  EXPECT_TRUE(group21->level() == Level::DEBUG);
  EXPECT_FALSE(group21->isLevelOverridden());
  EXPECT_TRUE(log3->sink() == sink2);
  EXPECT_TRUE(log3->level() == Level::DEBUG);

  EXPECT_TRUE(group22->parent() == group11);
  EXPECT_TRUE(group22->sink() == sink1);
  EXPECT_FALSE(group22->isSinkOverridden());
  EXPECT_TRUE(group22->level() == Level::TRACE);
  EXPECT_FALSE(group22->isLevelOverridden());
  EXPECT_TRUE(log4->sink() == sink1);
  EXPECT_TRUE(log4->level() == Level::TRACE);

  EXPECT_TRUE(group31->parent() == group21);
  EXPECT_TRUE(group31->sink() == sink3);
  EXPECT_TRUE(group31->isSinkOverridden());
  EXPECT_TRUE(group31->level() == Level::DEBUG);
  EXPECT_FALSE(group31->isLevelOverridden());
  EXPECT_TRUE(log5->sink() == sink3);
  EXPECT_TRUE(log5->level() == Level::DEBUG);

  EXPECT_TRUE(group32->parent() == group21);
  EXPECT_TRUE(group32->sink() == sink2);
  EXPECT_FALSE(group32->isSinkOverridden());
  EXPECT_TRUE(group32->level() == Level::CRITICAL);
  EXPECT_TRUE(group32->isLevelOverridden());
  EXPECT_TRUE(log6->sink() == sink2);
  EXPECT_TRUE(log6->level() == Level::CRITICAL);

  /// @When unsetting the parent of a third-level group
  system_->unsetParentOfGroup("third1");

  /// @Then the group should become independent (root-level)
  //
  // Hierarchy of groups
  //    G11 -- G22
  //    G12 -- G21 -- G32
  //    G31
  //
  // State of groups
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G12    G11    -      G21
  //  Sink:    S1     S2     @12=S2 @11=S1 =S3    @21=S2
  //  Level:   T      D      @12=D  G11=T  D      =C
  //                                       ^

  EXPECT_TRUE(group11->parent() == nullptr);
  EXPECT_TRUE(group11->sink() == sink1);
  EXPECT_FALSE(group11->isSinkOverridden());
  EXPECT_TRUE(group11->level() == Level::TRACE);
  EXPECT_FALSE(group11->isLevelOverridden());
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_TRUE(log1->level() == Level::TRACE);

  EXPECT_TRUE(group12->parent() == nullptr);
  EXPECT_TRUE(group12->sink() == sink2);
  EXPECT_FALSE(group12->isSinkOverridden());
  EXPECT_TRUE(group12->level() == Level::DEBUG);
  EXPECT_FALSE(group12->isLevelOverridden());
  EXPECT_TRUE(log2->sink() == sink2);
  EXPECT_TRUE(log2->level() == Level::DEBUG);

  EXPECT_TRUE(group21->parent() == group12);
  EXPECT_TRUE(group21->sink() == sink2);
  EXPECT_FALSE(group21->isSinkOverridden());
  EXPECT_TRUE(group21->level() == Level::DEBUG);
  EXPECT_FALSE(group21->isLevelOverridden());
  EXPECT_TRUE(log3->sink() == sink2);
  EXPECT_TRUE(log3->level() == Level::DEBUG);

  EXPECT_TRUE(group22->parent() == group11);
  EXPECT_TRUE(group22->sink() == sink1);
  EXPECT_FALSE(group22->isSinkOverridden());
  EXPECT_TRUE(group22->level() == Level::TRACE);
  EXPECT_FALSE(group22->isLevelOverridden());
  EXPECT_TRUE(log4->sink() == sink1);
  EXPECT_TRUE(log4->level() == Level::TRACE);

  EXPECT_TRUE(group31->parent() == nullptr);
  EXPECT_TRUE(group31->sink() == sink3);
  EXPECT_TRUE(group31->isSinkOverridden());
  EXPECT_TRUE(group31->level() == Level::DEBUG);
  EXPECT_FALSE(group31->isLevelOverridden());

  EXPECT_TRUE(log5->sink() == sink3);
  EXPECT_TRUE(log5->level() == Level::DEBUG);

  EXPECT_TRUE(group32->parent() == group21);
  EXPECT_TRUE(group32->sink() == sink2);
  EXPECT_FALSE(group32->isSinkOverridden());
  EXPECT_TRUE(group32->level() == Level::CRITICAL);
  EXPECT_TRUE(group32->isLevelOverridden());
  EXPECT_TRUE(log6->sink() == sink2);
  EXPECT_TRUE(log6->level() == Level::CRITICAL);

  /// @When changing the level of a group
  system_->setLevelOfGroup("second1", Level::INFO);

  /// @Then the level should be updated accordingly
  //
  // Hierarchy of groups
  //    G11 -- G22
  //    G12 -- G21 -- G32
  //    G31
  //
  // State of groups
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G12    G11    -      G21
  //  Sink:    S1     S2     @12=S2 @11=S1 =S3     @21=S2
  //  Level:   T      D      I      G11=T  D      =C
  //                         ^

  EXPECT_TRUE(group11->parent() == nullptr);
  EXPECT_TRUE(group11->sink() == sink1);
  EXPECT_FALSE(group11->isSinkOverridden());
  EXPECT_TRUE(group11->level() == Level::TRACE);
  EXPECT_FALSE(group11->isLevelOverridden());
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_TRUE(log1->level() == Level::TRACE);

  EXPECT_TRUE(group12->parent() == nullptr);
  EXPECT_TRUE(group12->sink() == sink2);
  EXPECT_FALSE(group12->isSinkOverridden());
  EXPECT_TRUE(group12->level() == Level::DEBUG);
  EXPECT_FALSE(group12->isLevelOverridden());
  EXPECT_TRUE(log2->sink() == sink2);
  EXPECT_TRUE(log2->level() == Level::DEBUG);

  EXPECT_TRUE(group21->parent() == group12);
  EXPECT_TRUE(group21->sink() == sink2);
  EXPECT_FALSE(group21->isSinkOverridden());
  EXPECT_TRUE(group21->level() == Level::INFO);
  EXPECT_TRUE(group21->isLevelOverridden());
  EXPECT_TRUE(log3->sink() == sink2);
  EXPECT_TRUE(log3->level() == Level::INFO);

  EXPECT_TRUE(group22->parent() == group11);
  EXPECT_TRUE(group22->sink() == sink1);
  EXPECT_FALSE(group22->isSinkOverridden());
  EXPECT_TRUE(group22->level() == Level::TRACE);
  EXPECT_FALSE(group22->isLevelOverridden());
  EXPECT_TRUE(log4->sink() == sink1);
  EXPECT_TRUE(log4->level() == Level::TRACE);

  EXPECT_TRUE(group31->parent() == nullptr);
  EXPECT_TRUE(group31->sink() == sink3);
  EXPECT_TRUE(group31->isSinkOverridden());
  EXPECT_TRUE(group31->level() == Level::DEBUG);
  EXPECT_FALSE(group31->isLevelOverridden());
  EXPECT_TRUE(log5->sink() == sink3);
  EXPECT_TRUE(log5->level() == Level::DEBUG);

  EXPECT_TRUE(group32->parent() == group21);
  EXPECT_TRUE(group32->sink() == sink2);
  EXPECT_FALSE(group32->isSinkOverridden());
  EXPECT_TRUE(group32->level() == Level::CRITICAL);
  EXPECT_TRUE(group32->isLevelOverridden());
  EXPECT_TRUE(log6->sink() == sink2);
  EXPECT_TRUE(log6->level() == Level::CRITICAL);

  /// @When change parent of group
  system_->setParentOfGroup("second1", "second2");

  /// @Then the properties should be inherited from the new parent
  //
  // Hierarchy of groups
  //    G11 -- G22 -- G21 -- G32
  //    G12
  //    G31
  //
  // State of groups
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  G32    -      G22    G11    -      G21
  //  Level:   T      D      C      G11=T  D      @21=C
  //  Sink:    S1     S2     @22=S1 @11=S1 S2     @21=S1
  //                         ^                    ^

  EXPECT_TRUE(group11->parent() == nullptr);
  EXPECT_TRUE(group11->sink() == sink1);
  EXPECT_FALSE(group11->isSinkOverridden());
  EXPECT_TRUE(group11->level() == Level::TRACE);
  EXPECT_FALSE(group11->isLevelOverridden());
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_TRUE(log1->level() == Level::TRACE);

  EXPECT_TRUE(group12->parent() == nullptr);
  EXPECT_TRUE(group12->sink() == sink2);
  EXPECT_FALSE(group12->isSinkOverridden());
  EXPECT_TRUE(group12->level() == Level::DEBUG);
  EXPECT_FALSE(group12->isLevelOverridden());
  EXPECT_TRUE(log2->sink() == sink2);
  EXPECT_TRUE(log2->level() == Level::DEBUG);

  EXPECT_TRUE(group21->parent() == group22);
  EXPECT_TRUE(group21->sink() == sink1);
  EXPECT_FALSE(group21->isSinkOverridden());
  EXPECT_TRUE(group21->level() == Level::INFO);
  EXPECT_TRUE(group21->isLevelOverridden());
  EXPECT_TRUE(log3->sink() == sink1);
  EXPECT_TRUE(log3->level() == Level::INFO);

  EXPECT_TRUE(group22->parent() == group11);
  EXPECT_TRUE(group22->sink() == sink1);
  EXPECT_FALSE(group22->isSinkOverridden());
  EXPECT_TRUE(group22->level() == Level::TRACE);
  EXPECT_FALSE(group22->isLevelOverridden());
  EXPECT_TRUE(log4->sink() == sink1);
  EXPECT_TRUE(log4->level() == Level::TRACE);

  EXPECT_TRUE(group31->parent() == nullptr);
  EXPECT_TRUE(group31->sink() == sink3);
  EXPECT_TRUE(group31->isSinkOverridden());
  EXPECT_TRUE(group31->level() == Level::DEBUG);
  EXPECT_FALSE(group31->isLevelOverridden());
  EXPECT_TRUE(log5->sink() == sink3);
  EXPECT_TRUE(log5->level() == Level::DEBUG);

  EXPECT_TRUE(group32->parent() == group21);
  EXPECT_TRUE(group32->sink() == sink1);
  EXPECT_FALSE(group32->isSinkOverridden());
  EXPECT_TRUE(group32->level() == Level::CRITICAL);
  EXPECT_TRUE(group32->isLevelOverridden());
  EXPECT_TRUE(log6->sink() == sink1);
  EXPECT_TRUE(log6->level() == Level::CRITICAL);
}

/**
 * @brief Tests changing the log level of individual loggers within the
 * LoggingSystem.
 *
 * @given A configured logging system with a single group and multiple loggers.
 * @when The log level of individual loggers is changed.
 * @then The new log level should be applied and marked as overridden, and
 * resetting should restore inheritance from the group.
 */
TEST_F(LoggingSystemTest, ChangeLevelOfLogger) {
  /// @Given a configured system with a single group and multiple loggers
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink1");
    system.makeGroup("group1", {}, "sink1", Level::INFO);
    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  auto group = system_->getGroup("group1");
  ASSERT_TRUE(group != nullptr);

  /// @Given loggers attached to the group
  auto log1 = system_->getLogger("Log1", "group1");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "group1", Level::DEBUG);
  ASSERT_TRUE(log2 != nullptr);

  /// @Given initial state:
  //
  // State of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   I
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   @1=I  D

  EXPECT_TRUE(log1->level() == Level::INFO);  // g1 - Inherits from group
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::DEBUG);  // own - Explicitly set
  EXPECT_TRUE(log2->isLevelOverridden());

  /// @When changing the log level of the loggers
  system_->setLevelOfLogger("Log1", Level::WARN);
  system_->setLevelOfLogger("Log2", Level::ERROR);

  /// @Then the new log levels should be applied and marked as overridden
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   W     E
  //           ^     ^

  EXPECT_TRUE(log1->level() == Level::WARN);  // *own - Explicitly set
  EXPECT_TRUE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::ERROR);  // *own - Explicitly set
  EXPECT_TRUE(log2->isLevelOverridden());

  /// @When changing the log level of the group
  system_->setLevelOfGroup("group1", Level::CRITICAL);

  /// @Then group log level should be updated, but loggers retain their
  ///       overridden values
  //
  // State of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   C
  //           ^

  /// @When resetting the log levels of the loggers
  system_->resetLevelOfLogger("Log1");
  system_->resetLevelOfLogger("Log2");

  /// @Then loggers should now inherit the new level from the group
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   C     C
  //           ^     ^

  EXPECT_TRUE(log1->level() == Level::CRITICAL);  // *g1 - Inherits from group
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::CRITICAL);  // *g1 - Inherits from group
  EXPECT_FALSE(log2->isLevelOverridden());
}

/**
 * @brief Tests changing the sink of individual loggers within the
 * LoggingSystem.
 *
 * @given A configured logging system with a single group and multiple sinks.
 * @when The sink of individual loggers is changed.
 * @then The new sink should be applied and marked as overridden, and resetting
 * should restore inheritance from the group.
 */
TEST_F(LoggingSystemTest, ChangeSinkOfLogger) {
  /// @Given a configured system with a single group and multiple sinks
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink1");
    system.makeSink<SinkMock>("sink2");
    system.makeSink<SinkMock>("sink3");
    system.makeSink<SinkMock>("sink4");
    system.makeSink<SinkMock>("sink5");
    system.makeGroup("group1", {}, "sink1", Level::INFO);
    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  auto group = system_->getGroup("group1");
  ASSERT_TRUE(group != nullptr);

  /// @Given available sinks
  auto sink1 = system_->getSink("sink1");
  ASSERT_TRUE(sink1 != nullptr);
  auto sink2 = system_->getSink("sink2");
  ASSERT_TRUE(sink2 != nullptr);
  auto sink3 = system_->getSink("sink3");
  ASSERT_TRUE(sink3 != nullptr);
  auto sink4 = system_->getSink("sink4");
  ASSERT_TRUE(sink4 != nullptr);
  auto sink5 = system_->getSink("sink5");
  ASSERT_TRUE(sink5 != nullptr);

  /// @Given loggers attached to the group
  auto log1 = system_->getLogger("Log1", "group1");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "group1", "sink2");
  ASSERT_TRUE(log2 != nullptr);

  /// @Given initial state:
  //
  // State of groups
  //  Group:   G1
  //  Parent:  -
  //  Sink:    S1

  // State of loggers
  //  Logger:  L1     L2
  //  Group:   G1     G1
  //  Level:   @1=S1  S2

  EXPECT_TRUE(log1->sink() == sink1);  // g1 - Inherits from group
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink2);  // own - Explicitly set
  EXPECT_TRUE(log2->isSinkOverridden());

  /// @When changing the sink of the loggers
  system_->setSinkOfLogger("Log1", "sink3");
  system_->setSinkOfLogger("Log2", "sink4");

  /// @Then the new sinks should be applied and marked as overridden
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   S3    S4
  //           ^     ^

  EXPECT_TRUE(log1->sink() == sink3);  // *own - Explicitly set
  EXPECT_TRUE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink4);  // *own - Explicitly set
  EXPECT_TRUE(log2->isSinkOverridden());

  /// @When changing the sink of the group
  system_->setSinkOfGroup("group1", "sink5");

  /// @Then group sink should be updated, but loggers retain their overridden
  ///       values
  //
  // State of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   S5
  //           ^

  /// @When resetting the sinks of the loggers
  system_->resetSinkOfLogger("Log1");
  system_->resetSinkOfLogger("Log2");

  /// @Then loggers should now inherit the new sink from the group
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   S5    S5
  //           ^     ^

  EXPECT_TRUE(log1->sink() == sink5);  // *g1 - Inherits from group
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink5);  // *g1 - Inherits from group
  EXPECT_FALSE(log2->isSinkOverridden());
}

/**
 * @brief Tests changing the group of loggers in the LoggingSystem.
 *
 * @given A configured logging system with two groups and multiple sinks.
 * @when The group of loggers is changed.
 * @then The new group should be applied, and non-overridden properties should
 * be inherited from the new group.
 */
TEST_F(LoggingSystemTest, ChangeGroupOfLogger) {
  /// @Given a configured system with two groups and multiple sinks
  ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillByDefault(Invoke([&](LoggingSystem &system) {
    system.makeSink<SinkMock>("sink1");
    system.makeSink<SinkMock>("sink2");
    system.makeSink<SinkMock>("sink3");
    system.makeGroup("first", {}, "sink1", Level::TRACE);
    system.makeGroup("second", {}, "sink2", Level::DEBUG);
    return Configurator::Result{};
  }));
  EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

  EXPECT_NO_THROW(auto r = system_->configure());

  /// @Given available sinks
  auto sink1 = system_->getSink("sink1");
  ASSERT_TRUE(sink1 != nullptr);
  auto sink2 = system_->getSink("sink2");
  ASSERT_TRUE(sink2 != nullptr);
  auto sink3 = system_->getSink("sink3");
  ASSERT_TRUE(sink3 != nullptr);

  /// @Given two groups
  auto group1 = system_->getGroup("first");
  ASSERT_TRUE(group1 != nullptr);
  auto group2 = system_->getGroup("second");
  ASSERT_TRUE(group2 != nullptr);

  /// @Given loggers attached to the first group
  auto log1 = system_->getLogger("Log1", "first");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "first", "sink3", Level::CRITICAL);
  ASSERT_TRUE(log2 != nullptr);

  /// @Given initial state:
  //
  // State of groups
  //  Group:   G1    G2
  //  Parent:  -     -
  //  Sink:    S1    S2
  //  Level:   T     D

  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Sink:    @1=S1 S3
  //  Level:   @1=T  C

  EXPECT_TRUE(log1->group() == group1);
  EXPECT_TRUE(log1->sink() == sink1);
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log1->level() == Level::TRACE);
  EXPECT_FALSE(log1->isLevelOverridden());

  EXPECT_TRUE(log2->group() == group1);
  EXPECT_TRUE(log2->sink() == sink3);
  EXPECT_TRUE(log2->isSinkOverridden());
  EXPECT_TRUE(log2->level() == Level::CRITICAL);
  EXPECT_TRUE(log2->isLevelOverridden());

  /// @When changing the group of loggers
  system_->setGroupOfLogger("Log1", "second");
  system_->setGroupOfLogger("Log2", "second");

  /// @Then loggers should inherit non-overridden properties from the new group
  //
  // State of loggers
  //  Logger:  L1    L2
  //  Group:   G2    G2
  //  Sink:    @2=S2 S3
  //  Level:   @1=D  C
  //             ^

  EXPECT_TRUE(log1->group() == group2);
  EXPECT_TRUE(log1->sink() == sink2);  // Inherited from new group
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log1->level() == Level::DEBUG);  // Inherited from new group
  EXPECT_FALSE(log1->isLevelOverridden());

  EXPECT_TRUE(log2->group() == group2);
  EXPECT_TRUE(log2->sink() == sink3);  // Still overridden
  EXPECT_TRUE(log2->isSinkOverridden());
  EXPECT_TRUE(log2->level() == Level::CRITICAL);  // Still overridden
  EXPECT_TRUE(log2->isLevelOverridden());
}
