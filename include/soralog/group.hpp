/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>

#include <soralog/level.hpp>
#include <soralog/logging_system.hpp>

namespace soralog {

  class Sink;

  /**
   * @class Group
   * @brief Manages logging properties such as level and sink.
   */
  class Group final {
   public:
    Group() = delete;
    Group(Group &&) noexcept = delete;
    Group(const Group &) = delete;
    virtual ~Group() = default;
    Group &operator=(Group &&) noexcept = delete;
    Group &operator=(const Group &) = delete;

    /**
     * @brief Creates a logging group.
     * @param logging_system Logging system reference.
     * @param group_name Group name.
     * @param parent_name Optional parent group name.
     * @param sink_name Optional sink name.
     * @param level Optional logging level.
     */
    Group(LoggingSystem &logging_system,
          std::string group_name,
          const std::optional<std::string> &parent_name,
          const std::optional<std::string> &sink_name,
          std::optional<Level> level);

    /**
     * @brief Gets the group's name.
     * @return Group name.
     */
    [[nodiscard]] const std::string &name() const noexcept {
      return name_;
    }

    // Level

    /**
     * @brief Gets the current logging level.
     * @return Logging level.
     */
    [[nodiscard]] Level level() const noexcept {
      return level_;
    }

    /**
     * @brief Checks if the level is overridden.
     * @return True if overridden, false if inherited.
     */
    [[nodiscard]] bool isLevelOverridden() const noexcept {
      return is_level_overridden_;
    }

    /**
     * @brief Resets level to inherit from parent.
     */
    void resetLevel();

    /**
     * @brief Sets the logging level.
     * @param level New logging level.
     */
    void setLevel(Level level);

    /**
     * @brief Sets level from another group.
     * @param group Source group.
     */
    void setLevelFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * @brief Sets level from a group by name.
     * @param group_name Name of source group.
     */
    void setLevelFromGroup(const std::string &group_name);

    // Sink

    /**
     * @brief Gets the current sink.
     * @return Shared pointer to sink.
     */
    [[nodiscard]] std::shared_ptr<const Sink> sink() const noexcept {
      return sink_;
    }

    /**
     * @brief Checks if the sink is overridden.
     * @return True if overridden, false if inherited.
     */
    [[nodiscard]] bool isSinkOverridden() const noexcept {
      return is_sink_overridden_;
    }

    /**
     * @brief Resets the sink to inherit from parent.
     */
    void resetSink();

    /**
     * @brief Sets the sink by name.
     * @param sink_name Name of the sink.
     */
    void setSink(const std::string &sink_name);

    /**
     * @brief Sets the sink.
     * @param sink Shared pointer to sink.
     */
    void setSink(std::shared_ptr<const Sink> sink);

    /**
     * @brief Sets the sink from another group.
     * @param group Source group.
     */
    void setSinkFromGroup(const std::shared_ptr<const Group> &group);

    /**
     * @brief Sets the sink from a group by name.
     * @param group_name Name of source group.
     */
    void setSinkFromGroup(const std::string &group_name);

    // Parent group

    /**
     * @brief Gets the parent group.
     * @return Shared pointer to parent group.
     */
    [[nodiscard]] std::shared_ptr<const Group> parent() const noexcept {
      return parent_group_;
    }

    /**
     * @brief Unsets the parent group.
     * All properties become overridden.
     */
    void unsetParentGroup();

    /**
     * @brief Sets the parent group.
     * Non-overridden properties inherit from it.
     * @param group New parent group.
     */
    void setParentGroup(std::shared_ptr<const Group> group);

    /**
     * @brief Sets the parent group by name.
     * Non-overridden properties inherit from it.
     * @param group_name Name of parent group.
     */
    void setParentGroup(const std::string &group_name);

   private:
    LoggingSystem &system_;   ///< Reference to logging system.
    const std::string name_;  ///< Group name.

    std::shared_ptr<const Group> parent_group_;  ///< Parent group.

    std::shared_ptr<const Sink> sink_;  ///< Logging sink.
    bool is_sink_overridden_{};         ///< Is sink overridden.

    Level level_{};               ///< Logging level.
    bool is_level_overridden_{};  ///< Is level overridden.
  };

}  // namespace soralog
