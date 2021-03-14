/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_GROUP
#define SORALOG_GROUP

#include <memory>
#include <string>

#include <soralog/level.hpp>
#include <soralog/logging_system.hpp>

namespace soralog {

  class Group final {
   public:
    Group() = delete;
    Group(Group &&) noexcept = delete;
    Group(const Group &) = delete;
    virtual ~Group() = default;
    Group &operator=(Group &&) noexcept = delete;
    Group &operator=(Group const &) = delete;

    Group(LoggingSystem &logging_system, std::string group_name,
          const std::optional<std::string> &parent_name,
          const std::optional<std::string> &sink_name,
          std::optional<Level> level);

    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    // Level

    [[nodiscard]] Level level() const noexcept {
      return level_;
    }
    [[nodiscard]] bool hasLevelOverriden() const noexcept {
      return has_level_overriden_;
    }

    void resetLevel();
    void setLevel(Level level);
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }
    [[nodiscard]] bool hasSinkOverriden() const noexcept {
      return has_sink_overriden_;
    }

    void resetSink();
    void setSink(std::shared_ptr<const Sink> group);
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    [[nodiscard]] std::shared_ptr<const Group> parent() const noexcept {
      return parent_group_;
    }

    void setParentGroup(std::shared_ptr<const Group> group);
    void setParentGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;
    const std::string name_;

    std::shared_ptr<const Group> parent_group_;

    std::shared_ptr<const Sink> sink_;
    bool has_sink_overriden_{};

    Level level_{};
    bool has_level_overriden_{};
  };

}  // namespace soralog

#endif  // SORALOG_GROUP
