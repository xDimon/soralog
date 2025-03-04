/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/logger_factory.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <soralog/configurator.hpp>

namespace soralog {

  class Sink;
  class Group;
  class Logger;

  /**
   * @class LoggingSystem
   * @brief Manages loggers, sinks, and groups, allowing dynamic configuration.
   *
   * The logging system tracks loggers, groups, and sinks, ensuring their
   * proper initialization and configuration. It provides methods to create,
   * retrieve, and modify logging entities at runtime.
   */
  class LoggingSystem final : public LoggerFactory {
   public:
    LoggingSystem(const LoggingSystem &) = delete;
    LoggingSystem &operator=(const LoggingSystem &) = delete;
    ~LoggingSystem() override = default;
    LoggingSystem(LoggingSystem &&tmp) noexcept = delete;
    LoggingSystem &operator=(LoggingSystem &&tmp) noexcept = delete;

    /**
     * @brief Constructs a non-configured logging system for manual configuring
     */
    LoggingSystem();

    /**
     * @brief Constructs a logging system with a configurator.
     * @param configurator Shared pointer to a configurator instance.
     */
    explicit LoggingSystem(std::shared_ptr<Configurator> configurator);

    /**
     * @brief Configures the logging system.
     * @return Result of the configuration process.
     */
    [[nodiscard]] Configurator::Result configure();

    // Logger retrieval methods

    /**
     * @brief Gets or creates a logger with a specified name and group.
     * @param logger_name Logger name.
     * @param group_name Associated group name.
     * @return Shared pointer to the logger.
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) override {
      return getLogger(
          std::move(logger_name), group_name, std::nullopt, std::nullopt);
    }

    /**
     * @brief Gets or creates a logger with an overridden log level.
     * @param logger_name Logger name.
     * @param group_name Associated group name.
     * @param level Log level override.
     * @return Shared pointer to the logger.
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        Level level) override {
      return getLogger(std::move(logger_name),
                       group_name,
                       std::nullopt,
                       std::make_optional(level));
    }

    /**
     * @brief Gets or creates a logger with an overridden sink.
     * @param logger_name Logger name.
     * @param group_name Associated group name.
     * @param sink_name Sink override.
     * @return Shared pointer to the logger.
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        std::string sink_name) override {
      return getLogger(std::move(logger_name),
                       group_name,
                       std::make_optional(std::move(sink_name)),
                       std::nullopt);
    }

    /**
     * @brief Gets or creates a logger with overridden sink and level.
     * @param logger_name Logger name.
     * @param group_name Associated group name.
     * @param sink_name Sink override.
     * @param level Log level override.
     * @return Shared pointer to the logger.
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        std::string sink_name,
        Level level) override {
      return getLogger(std::move(logger_name),
                       group_name,
                       std::make_optional(std::move(sink_name)),
                       std::make_optional(level));
    }

    // Sink and group management

    /**
     * @brief Retrieves a sink by name.
     * @param name Sink name.
     * @return Shared pointer to the sink, or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &name);

    /**
     * @brief Retrieves a group by name.
     * @param name Group name.
     * @return Shared pointer to the group, or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<Group> getGroup(const std::string &name);

    /**
     * @brief Creates a new sink of a specified type.
     * @tparam SinkType Type of the sink.
     * @tparam Args Constructor argument types.
     * @param args Arguments for the sink constructor.
     * @return Shared pointer to the created sink.
     */
    template <typename SinkType, typename... Args>
    std::shared_ptr<SinkType> makeSink(Args &&...args) {
      std::lock_guard guard(mutex_);
      auto sink = std::make_shared<SinkType>(std::forward<Args>(args)...);
      sinks_[sink->name()] = sink;
      return sink;
    }

    /**
     * @brief Creates a logging group.
     * @param name Group name.
     * @param parent Optional parent group name.
     * @param sink Optional sink override.
     * @param level Optional log level override.
     * @return Shared pointer to the created group.
     */
    std::shared_ptr<Group> makeGroup(std::string name,
                                     const std::optional<std::string> &parent,
                                     const std::optional<std::string> &sink,
                                     const std::optional<Level> &level);

    /**
     * @brief Declares or changes the default logging group.
     * @param group_name Fallback group name.
     * @return True if successful.
     */
    bool setFallbackGroup(const std::string &group_name);

    /**
     * @brief Retrieves the fallback group.
     * @return Shared pointer to the fallback group.
     */
    std::shared_ptr<Group> getFallbackGroup() const;

    /**
    * @brief Sets the parent of a group.
     * @param group_name Name of the group.
     * @param parent Name of the parent group.
     * @return True if successful.
     */
    bool setParentOfGroup(const std::string &group_name,
                          const std::string &parent);

    /**
     * @brief Unsets the parent of a group.
     * @param group_name Name of the group.
     * @return True if successful.
     */
    bool unsetParentOfGroup(const std::string &group_name);

    /**
     * @brief Sets the sink of a group.
     * @param group_name Name of the group.
     * @param sink_name Name of the sink.
     * @return True if successful.
     */
    bool setSinkOfGroup(const std::string &group_name,
                        const std::string &sink_name);

    /**
     * @brief Resets the sink of a group to its parent's sink.
     * @param group_name Name of the group.
     * @return True if successful.
     */
    bool resetSinkOfGroup(const std::string &group_name);

    /**
     * @brief Sets the logging level of a group.
     * @param group_name Name of the group.
     * @param level New logging level.
     * @return True if successful.
     */
    bool setLevelOfGroup(const std::string &group_name, Level level);

    /**
     * @brief Resets the logging level of a group to its parent's level.
     * @param group_name Name of the group.
     * @return True if successful.
     */
    bool resetLevelOfGroup(const std::string &group_name);

    /**
     * @brief Sets the group of a logger.
     * The logger will inherit properties from the specified group.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group to assign.
     * @return True if successful.
     */
    bool setGroupOfLogger(const std::string &logger_name,
                          const std::string &group_name);

    /**
     * @brief Overrides a logger's sink.
     * @param logger_name Logger name.
     * @param sink_name Sink override.
     * @return True if successful.
     */
    bool setSinkOfLogger(const std::string &logger_name,
                         const std::string &sink_name);

    /**
     * @brief Resets a logger's sink to match its group's.
     * Marks the sink as inherited.
     * @param logger_name Logger name.
     * @return True if successful.
     */
    bool resetSinkOfLogger(const std::string &logger_name);

    /**
     * @brief Overrides a logger's logging level.
     * @param logger_name Logger name.
     * @param level Log level override.
     * @return True if successful.
     */
    bool setLevelOfLogger(const std::string &logger_name, Level level);

    /**
     * @brief Resets a logger's logging level to match its group's.
     * Marks the level as inherited.
     * @param logger_name Logger name.
     * @return True if successful.
     */
    bool resetLevelOfLogger(const std::string &logger_name);

    /**
     * @brief Calls `rotate()` on all registered sinks.
     */
    void callRotateForAllSinks();

   private:
    /**
     * @brief Retrieves or creates a logger with optional sink and level
     * overrides.
     * @param logger_name Name of the logger.
     * @param group_name Name of the group.
     * @param sink_name Optional sink name override.
     * @param level Optional log level override.
     * @return Shared pointer to the logger.
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name,
        const std::string &group_name,
        const std::optional<std::string> &sink_name,
        const std::optional<Level> &level);

    /**
     * @brief Sets or unsets the parent group for a given group.
     * @param group The group to modify.
     * @param parent The new parent group, or null to unset.
     */
    void setParentOfGroup(const std::shared_ptr<Group> &group,
                          const std::shared_ptr<Group> &parent);

    /**
     * @brief Sets or unsets the sink for a given group.
     * @param group The group to modify.
     * @param sink The new sink, or null to reset.
     */
    void setSinkOfGroup(const std::shared_ptr<Group> &group,
                        std::optional<std::shared_ptr<Sink>> sink);

    /**
     * @brief Sets or unsets the level for a given group.
     * @param group The group to modify.
     * @param level The new log level, or null to reset.
     */
    void setLevelOfGroup(const std::shared_ptr<Group> &group,
                         std::optional<Level> level);

    /**
     * @brief Sets or unsets the sink for a logger.
     * @param logger The logger to modify.
     * @param sink The new sink, or null to reset.
     */
    static void setSinkOfLogger(const std::shared_ptr<Logger> &logger,
                                std::optional<std::shared_ptr<Sink>> sink);

    /**
     * @brief Sets or unsets the level for a logger.
     * @param logger The logger to modify.
     * @param level The new log level, or null to reset.
     */
    static void setLevelOfLogger(const std::shared_ptr<Logger> &logger,
                                 std::optional<Level> level);

    /// Logging system configurator.
    std::shared_ptr<Configurator> configurator_;
    /// Flag indicating if the system is configured.
    bool is_configured_ = false;
    /// Mutex for thread-safe access.
    std::recursive_mutex mutex_;

    /// Logger storage.
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
    /// Sink storage.
    std::unordered_map<std::string, std::shared_ptr<Sink>> sinks_;
    /// Group storage.
    std::unordered_map<std::string, std::shared_ptr<Group>> groups_;
  };

}  // namespace soralog
