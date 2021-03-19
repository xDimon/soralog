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

  class Sink;

  /**
   * @class Group
   * Entity to save and distribute properties
   */
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

    /**
     * @returns name of group
     */
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    // Level

    /**
     * @returns current level of logging
     */
    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    /**
     * @returns true if level is overridden, and true if it is inherited from
     * group
     */
    [[nodiscard]] bool isLevelOverridden() const noexcept {
      return is_level_overridden_;
    }

    /**
     * Set level to level from group. Level will be marked as inherited
     */
    void resetLevel();

    /**
     * Set level to {@param level}. Level will be marked as overridden
     */
    void setLevel(Level level);

    /**
     * Set level to level from group {@param group}
     */
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * Set level to level from group with name {@param group_name}
     */
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    /**
     * @returns current sink of logging
     */
    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }

    /**
     * @returns true if sink is overridden, and true if it is inherited from
     * group
     */
    [[nodiscard]] bool isSinkOverridden() const noexcept {
      return is_sink_overridden_;
    }

    /**
     * Set sink to sink from group. Level will be marked as inherited
     */
    void resetSink();

    /**
     * Set sink to sink with name {@param sink_name}.
     * Level will be marked as overridden
     */
    void setSink(const std::string &sink_name);

    /**
     * Set sink to sink {@param sink}. Level will be marked as overridden
     */
    void setSink(std::shared_ptr<const Sink> sink);

    /**
     * Set sink to sink from group {@param group}
     */
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * Set sink to sink from group with name {@param group_name}
     */
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    /**
     * @returns parent group
     */
    [[nodiscard]] std::shared_ptr<const Group> parent() const noexcept {
      return parent_group_;
    }

    /**
     * Unset parent group.
     * All properties will be marked as overridden
     */

    void unsetParentGroup();

    /**
     * Set parent group to {@param group}.
     * Non overridden properties will be inherited from new parent
     */
    void setParentGroup(std::shared_ptr<const Group> group);

    /**
     * Set parent group to group with name {@param group_name}.
     * Non overridden properties will be inherited from new parent
     */
    void setParentGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;
    const std::string name_;

    std::shared_ptr<const Group> parent_group_;

    std::shared_ptr<const Sink> sink_;
    bool is_sink_overridden_{};

    Level level_{};
    bool is_level_overridden_{};
  };

}  // namespace soralog

#endif  // SORALOG_GROUP
