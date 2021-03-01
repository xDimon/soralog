/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_GROUP
#define SORALOG_GROUP

#include <memory>
#include <string>

#include <log_levels.hpp>
#include <logger_system.hpp>

namespace soralog {

  class Group final {
   public:
    Group() = delete;
    Group(Group &&) noexcept = delete;
    Group(const Group &) = delete;
    virtual ~Group() = default;
    Group &operator=(Group &&) noexcept = delete;
    Group &operator=(Group const &) = delete;

    Group(LoggerSystem &logger_system, std::string group_name,
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
    [[nodiscard]] bool isLevelOverriden() const noexcept {
      return level_is_overriden_;
    }

    void resetLevel();
    void setLevel(Level level);
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }
    [[nodiscard]] bool isSinkOverriden() const noexcept {
      return sink_is_overriden_;
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
    LoggerSystem &system_;
    const std::string name_;

    std::shared_ptr<const Group> parent_group_;

    std::shared_ptr<const Sink> sink_;
    bool sink_is_overriden_{};

    Level level_{};
    bool level_is_overriden_{};
  };

}  // namespace soralog

#endif  // SORALOG_GROUP
