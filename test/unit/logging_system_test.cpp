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

class LoggingSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    configurator_ = std::make_shared<ConfiguratorMock>();
    system_ = std::make_shared<LoggingSystem>(configurator_);
  }
  void TearDown() override {}

  void configure() {
    ON_CALL(*configurator_, applyOn(Truly([&](auto &s) {
      return &s == system_.get();
    }))).WillByDefault(Invoke([&](LoggingSystem &system) {
      auto sink1 = system.makeSink<SinkMock>("sink");
      EXPECT_CALL(*sink1, mocked_flush()).Times(testing::AnyNumber());
      auto sink2 = system.makeSink<SinkMock>("other");
      EXPECT_CALL(*sink2, mocked_flush()).Times(testing::AnyNumber());
      system.makeGroup("first", {}, "sink", Level::VERBOSE);
      system.makeGroup("second", "first", "sink", Level::DEBUG);
      system.makeGroup("third", "second", "sink", Level::TRACE);
      return Configurator::Result{};
    }));
    EXPECT_CALL(*configurator_, applyOn(_)).Times(1);

    EXPECT_NO_THROW(auto r = system_->configure());
  }

 protected:
  std::shared_ptr<ConfiguratorMock> configurator_;
  std::shared_ptr<LoggingSystem> system_;
};

TEST_F(LoggingSystemTest, Configure) {
  EXPECT_CALL(*configurator_, applyOn(Truly([&](auto &s) {
    return &s == system_.get();
  }))).WillRepeatedly(Return(Configurator::Result{}));

  EXPECT_TRUE(system_->getSink("*") != nullptr);

  EXPECT_NO_THROW(auto r = system_->configure());

  EXPECT_THROW(auto r = system_->configure(), std::logic_error);
}

TEST_F(LoggingSystemTest, MakeGroup) {
  EXPECT_TRUE(system_->getFallbackGroup() == nullptr);

  EXPECT_ANY_THROW(system_->makeGroup("first", {}, {}, {}));
  EXPECT_NO_THROW(system_->makeGroup("first", {}, {}, Level::INFO));

  auto defGroup = system_->getFallbackGroup();
  auto firstGroup = system_->getGroup("first");
  ASSERT_TRUE(defGroup != nullptr);
  EXPECT_TRUE(firstGroup == defGroup);

  EXPECT_ANY_THROW(system_->makeGroup("second", "nonexisting_group", {}, {}));
  EXPECT_ANY_THROW(system_->makeGroup("second", {}, "nonexisting_sink", {}));
  EXPECT_NO_THROW(system_->makeGroup("second", "first", {}, {}));
}

TEST_F(LoggingSystemTest, MakeSink) {
  EXPECT_FALSE(system_->getSink("sink") != nullptr);

  EXPECT_NO_THROW(system_->makeSink<SinkMock>("sink"));

  EXPECT_TRUE(system_->getSink("sink") != nullptr);
}

TEST_F(LoggingSystemTest, GetGroup) {
  configure();

  EXPECT_TRUE(system_->getFallbackGroup() != nullptr);

  auto firstGroup = system_->getGroup("first");
  EXPECT_TRUE(firstGroup != nullptr);

  auto secondGroup = system_->getGroup("second");
  EXPECT_TRUE(secondGroup != nullptr);

  auto thirdGroup = system_->getGroup("third");
  EXPECT_TRUE(thirdGroup != nullptr);

  auto fourthGroup = system_->getGroup("fourth");
  EXPECT_FALSE(fourthGroup != nullptr);
}

TEST_F(LoggingSystemTest, GetSink) {
  EXPECT_TRUE(system_->getSink("*") != nullptr);

  EXPECT_FALSE(system_->getSink("sink") != nullptr);

  configure();

  EXPECT_TRUE(system_->getSink("sink") != nullptr);
}

TEST_F(LoggingSystemTest, GetLogger) {
  configure();

  // Default group is used instead unexisting one
  {
    auto log0 = system_->getLogger("Log_0", "nonexisting_group");
    ASSERT_TRUE(log0 != nullptr);
    auto group = system_->getFallbackGroup();
    ASSERT_TRUE(group != nullptr);
    EXPECT_TRUE(log0->group() == group);
    EXPECT_TRUE(log0->sink() == group->sink());
    EXPECT_TRUE(log0->level() == group->level());
  }

  // Gotten from his group
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

  // Custom sink
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

  // Custom level
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

  // Custom sink and level
  {
    auto log5 = system_->getLogger("Log_6", "first", "other", Level::INFO);
    ASSERT_TRUE(log5 != nullptr);
    auto group = log5->group();
    ASSERT_TRUE(group != nullptr);
    auto sink = log5->sink();
    ASSERT_TRUE(sink != nullptr);
    auto level = log5->level();
    ASSERT_TRUE(group == system_->getGroup("first"));
    EXPECT_TRUE(sink != group->sink());
    EXPECT_TRUE(sink == system_->getSink("other"));
    EXPECT_TRUE(level != group->level());
    EXPECT_TRUE(level == Level::INFO);
  }
}

TEST_F(LoggingSystemTest, FallbackGroup) {
  EXPECT_TRUE(system_->getFallbackGroup() == nullptr);

  configure();

  auto group1 = system_->getGroup("first");
  EXPECT_TRUE(group1 != nullptr);

  auto group2 = system_->getGroup("second");
  EXPECT_TRUE(group2 != nullptr);

  EXPECT_TRUE(system_->getFallbackGroup() == group1);

  system_->setFallbackGroup("second");
  EXPECT_TRUE(system_->getFallbackGroup() == group2);
}

TEST_F(LoggingSystemTest, ChangeLevelOfGroup) {
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

  /// @Given beginning state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   I     @1=I  W

  /// @Given beginning state of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Level:   @1=I  T     @2=I  D     @3=W  V

  expect_initial_state();

  /// @When change level of top group
  system_->setLevelOfGroup("first", Level::CRITICAL);

  /// @Then is have next state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   C     @1=C  W
  //           ^        ^

  /// @Then is have next state of loggers
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

  /// @When coming back previous group's level
  system_->setLevelOfGroup("first", Level::INFO);

  /// @Than is came back to beginning state
  expect_initial_state();

  /// @When changing level of dependent group
  system_->setLevelOfGroup("second", Level::CRITICAL);

  /// @Then is have next state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Level:   I     C     W
  //                 ^

  /// @Then is have next state of loggers
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

  /// @When come back dependent of top group
  system_->resetLevelOfGroup("second");

  /// @Than is came back to beginning state
  expect_initial_state();
}

TEST_F(LoggingSystemTest, ChangeSinkOfGroup) {
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

  /// @Given beginning state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S1    @1=S1 S2

  /// @Given beginning state of loggers
  //  Logger:  L1    L2    L3    L4    L5    L6
  //  Group:   G1    G1    G2    G2    G3    G4
  //  Sink:    @1=S1 S3    @2=S1 S4    @3=S2 S5

  expect_initial_state();

  /// @When change sink of top group
  system_->setSinkOfGroup("first", "sink6");

  /// @Then is have next state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S6    @1=S6 S2
  //           ^        ^

  /// @Then is have next state of loggers
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

  /// @When come back sink of top group
  system_->setSinkOfGroup("first", "sink1");

  /// @Than is came back to beginning state
  expect_initial_state();

  /// @When change sink of dependent group
  system_->setSinkOfGroup("second", "sink6");

  /// @Then is have next state of groups
  //  Group:   G1    G2    G3
  //  Parent:  -     G1    G2
  //  Sink:    S1    S6    S2
  //                 ^

  /// @Then is have next state of loggers
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

  /// @When come back dependent of top group
  system_->resetSinkOfGroup("second");

  /// @Than is came back to beginning state
  expect_initial_state();
}

TEST_F(LoggingSystemTest, ChangeParentGroup) {
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

  // Create loggers
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

  /// @Given beginning state of groups
  //    G11 -- G21 -- G31
  //        \      `- G32
  //         ` G22
  //    G12
  //
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G11    G11    G21    G21
  //  Sink:    S1     S2     @11=S1 @11=S1 =S3 @21=S1
  //  Level:   T      D      @11=T  G11=T  @21=T  =C

  /// @Given beginning state of loggers
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

  /// @When change parent of 2-level group
  system_->setParentOfGroup("second1", "first2");

  /// @Then is have next state of groups
  //    G11 -- G22
  //    G12 -- G21 -- G31
  //               `- G32
  //
  //  Group:   G11    G12    G21    G22    G31    G32
  //  Parent:  -      -      G12    G11    G21    G21
  //  Sink:    S1     S2     @12=S2 @11=S1 =S3 @21=S2
  //  Level:   T      D      @12=D  G11=T  @21=D  =C
  //                  ^                ^      ^

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

  /// @When unset parent of group
  system_->unsetParentOfGroup("third1");

  /// @Then is have next state of groups
  //    G11 -- G22
  //    G12 -- G21 -- G32
  //    G31
  //
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

  /// @When change level of group
  system_->setLevelOfGroup("second1", Level::INFO);

  /// @Then is have next state of groups
  //    G11 -- G22
  //    G12 -- G21 -- G32
  //    G31
  //
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

  /// @Then is have next state of groups
  //    G11 -- G22 -- G21 -- G32
  //    G12
  //    G31
  //
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

TEST_F(LoggingSystemTest, ChangeLevelOfLogger) {
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

  // Create loggers
  auto log1 = system_->getLogger("Log1", "group1");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "group1", Level::DEBUG);
  ASSERT_TRUE(log2 != nullptr);

  /// @Given beginning state of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   I

  /// @Given beginning state of loggers
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   @1=I  D

  EXPECT_TRUE(log1->level() == Level::INFO);  // g1
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::DEBUG);  // own
  EXPECT_TRUE(log2->isLevelOverridden());

  /// @When change level of loggers
  system_->setLevelOfLogger("Log1", Level::WARN);
  system_->setLevelOfLogger("Log2", Level::ERROR);

  /// @Then is have next state of groups
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   W     E
  //           ^     ^

  EXPECT_TRUE(log1->level() == Level::WARN);  // *own
  EXPECT_TRUE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::ERROR);  // *own
  EXPECT_TRUE(log2->isLevelOverridden());

  /// @When change level of loggers
  system_->setLevelOfGroup("group1", Level::CRITICAL);

  /// @Then is have next state of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   C
  //           ^

  /// @When reset levels of loggers
  system_->resetLevelOfLogger("Log1");
  system_->resetLevelOfLogger("Log2");

  /// @Then is have next state of groups
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   C     C
  //           ^     ^

  EXPECT_TRUE(log1->level() == Level::CRITICAL);  // *g1
  EXPECT_FALSE(log1->isLevelOverridden());
  EXPECT_TRUE(log2->level() == Level::CRITICAL);  // *g1
  EXPECT_FALSE(log2->isLevelOverridden());
}

TEST_F(LoggingSystemTest, ChangeSinkOfLogger) {
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

  // Create loggers
  auto log1 = system_->getLogger("Log1", "group1");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "group1", "sink2");
  ASSERT_TRUE(log2 != nullptr);

  /// @Given beginning state of groups
  //  Group:   G1
  //  Parent:  -
  //  Sink:    S1

  /// @Given beginning state of loggers
  //  Logger:  L1     L2
  //  Group:   G1     G1
  //  Level:   @1=S1  S2

  EXPECT_TRUE(log1->sink() == sink1);  // g1
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink2);  // own
  EXPECT_TRUE(log2->isSinkOverridden());

  /// @When change sink of loggers
  system_->setSinkOfLogger("Log1", "sink3");
  system_->setSinkOfLogger("Log2", "sink4");

  /// @Then is have next state of groups
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   S3    S4
  //           ^     ^

  EXPECT_TRUE(log1->sink() == sink3);  // *own
  EXPECT_TRUE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink4);  // *own
  EXPECT_TRUE(log2->isSinkOverridden());

  /// @When change sink of loggers
  system_->setSinkOfGroup("group1", "sink5");

  /// @Then is have next state of groups
  //  Group:   G1
  //  Parent:  -
  //  Level:   S5
  //           ^

  /// @When reset sink of loggers
  system_->resetSinkOfLogger("Log1");
  system_->resetSinkOfLogger("Log2");

  /// @Then is have next state of groups
  //  Logger:  L1    L2
  //  Group:   G1    G1
  //  Level:   S5    S5
  //           ^     ^

  EXPECT_TRUE(log1->sink() == sink5);  // *g1
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log2->sink() == sink5);  // *g1
  EXPECT_FALSE(log2->isSinkOverridden());
}

TEST_F(LoggingSystemTest, ChangeGroupOfLogger) {
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

  auto sink1 = system_->getSink("sink1");
  ASSERT_TRUE(sink1 != nullptr);
  auto sink2 = system_->getSink("sink2");
  ASSERT_TRUE(sink2 != nullptr);
  auto sink3 = system_->getSink("sink3");
  ASSERT_TRUE(sink3 != nullptr);

  auto group1 = system_->getGroup("first");
  ASSERT_TRUE(group1 != nullptr);
  auto group2 = system_->getGroup("second");
  ASSERT_TRUE(group2 != nullptr);

  // Create loggers
  auto log1 = system_->getLogger("Log1", "first");
  ASSERT_TRUE(log1 != nullptr);
  auto log2 = system_->getLogger("Log2", "first", "sink3", Level::CRITICAL);
  ASSERT_TRUE(log2 != nullptr);

  /// @Given beginning state of groups
  //  Group:   G1    G2
  //  Parent:  -     -
  //  Sink:    S1    S2
  //  Level:   T     D

  /// @Given beginning state of loggers
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

  /// @When change group of loggers
  system_->setGroupOfLogger("Log1", "second");
  system_->setGroupOfLogger("Log2", "second");

  /// @Then is have next state of loggers
  //  Logger:  L1    L2
  //  Group:   G2    G2
  //  Sink:    @2=S2 S3
  //  Level:   @1=D  C
  //             ^

  EXPECT_TRUE(log1->group() == group2);
  EXPECT_TRUE(log1->sink() == sink2);
  EXPECT_FALSE(log1->isSinkOverridden());
  EXPECT_TRUE(log1->level() == Level::DEBUG);
  EXPECT_FALSE(log1->isLevelOverridden());

  EXPECT_TRUE(log2->group() == group2);
  EXPECT_TRUE(log2->sink() == sink3);
  EXPECT_TRUE(log2->isSinkOverridden());
  EXPECT_TRUE(log2->level() == Level::CRITICAL);
  EXPECT_TRUE(log2->isLevelOverridden());
}
