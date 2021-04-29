/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGINGSYSTEM
#define SORALOG_LOGGINGSYSTEM

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
   * Holds created sinks and group and knowns created loggers to control his
   * proterties
   */
  class LoggingSystem final : public LoggerFactory {
   public:
    LoggingSystem() = delete;
    LoggingSystem(const LoggingSystem &) = delete;
    LoggingSystem &operator=(LoggingSystem const &) = delete;
    ~LoggingSystem() override = default;
    LoggingSystem(LoggingSystem &&tmp) noexcept = delete;
    LoggingSystem &operator=(LoggingSystem &&tmp) noexcept = delete;

    explicit LoggingSystem(std::shared_ptr<Configurator> configurator);

    /**
     * Call configurator to setup logging system for work
     * @return result of configure
     */
    [[nodiscard]] Configurator::Result configure();

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name} and group {@param group_name}
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) override {
      return getLogger(std::move(logger_name), group_name, std::nullopt,
                       std::nullopt);
    }

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and level
     * overridden to {@param level}
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        Level level) override {
      return getLogger(std::move(logger_name), group_name, std::nullopt,
                       std::make_optional(level));
    }

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and sink
     * overridden to sink with name {@param sink_name}
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name) override {
      return getLogger(std::move(logger_name), group_name,
                       std::make_optional(std::move(sink_name)), std::nullopt);
    }

    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}, and sink
     * andd level overridden to {@param sink_name} and {@param level}
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name, Level level) override {
      return getLogger(std::move(logger_name), group_name,
                       std::make_optional(std::move(sink_name)),
                       std::make_optional(level));
    }

    /**
     * @returns sink with name {@param name}
     */
    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &name);

    /**
     * @returns group with name {@param name}
     */
    [[nodiscard]] std::shared_ptr<Group> getGroup(const std::string &name);

    /**
     * Creates sink with type {@tparam SinkType} using arguments {@param args}
     */
    template <typename SinkType, typename... Args>
    std::shared_ptr<SinkType> makeSink(Args &&... args) {
      std::lock_guard guard(mutex_);
      auto sink = std::make_shared<SinkType>(std::forward<Args>(args)...);
      sinks_[sink->name()] = sink;
      return sink;
    }

    /**
     * Creates group with name {@param name}
     * @param parent - group from which sink and level are inherited
     * @param sink - overriding sink if provided
     * @param level - overriding level if provided
     * @note Sink and level must be provided if parent is not
     */
    std::shared_ptr<Group> makeGroup(std::string name,
                                     const std::optional<std::string> &parent,
                                     const std::optional<std::string> &sink,
                                     const std::optional<Level> &level);

    /**
     * Declares/changes default group to group with name {@param group_name}
     * @returns true if any
     */
    bool setFallbackGroup(const std::string &group_name);

    /**
     * @returns fallback group
     */
    std::shared_ptr<Group> getFallbackGroup() const;

    /**
     * Set parent group of group {@param group_name} to {@param parent}.
     * Inherited properties will be replaced by parent's
     * @returns true if success
     */
    bool setParentOfGroup(const std::string &group_name,
                          const std::string &parent);
    /**
     * Unset parent group of group {@param group_name}.
     * All properties will be marked as overridden (stay his own)
     * @returns true if success
     */
    bool unsetParentOfGroup(const std::string &group_name);

    /**
     * Set sink of group {@param group_name} to sink with name {@param
     * sink_name}. Sink will be marked as overridden
     * @returns true if success
     */
    bool setSinkOfGroup(const std::string &group_name,
                        const std::string &sink_name);

    /**
     * Set sink of group {@param group_name} to sink of parent group, if that
     * defined
     * @returns true if success
     */
    bool resetSinkOfGroup(const std::string &group_name);

    /**
     * Set level of group {@param group_name} to level {@param level}. Level
     * will be marked as overridden
     * @returns true if success
     */
    bool setLevelOfGroup(const std::string &group_name, Level level);

    /**
     * Set level of group {@param group_name} to level of parent group, if that
     * defined
     * @returns true if success
     */
    bool resetLevelOfGroup(const std::string &group_name);

    /**
     * Set group of logger {@param logger_name} to group with name {@param
     * group_name}. Inherited properties will be replaced by group's
     * @returns true if success
     */
    bool setGroupOfLogger(const std::string &logger_name,
                          const std::string &group_name);

    /**
     * Set sink of logger {@param logger_name} to sink with name {@param
     * group_name}. Sink will be marked as overridden.
     * @returns true if success
     */
    bool setSinkOfLogger(const std::string &logger_name,
                         const std::string &sink_name);

    /**
     * Set sink of logger with nam {@param group_name} to sink of his group.
     * Sink will be marked as inherited.
     * @returns true if success
     */
    bool resetSinkOfLogger(const std::string &logger_name);

    /**
     * Set sink of logger {@param logger_name} to sink with name {@param
     * group_name}. Sink will be marked as overridden.
     * @returns true if success
     */
    bool setLevelOfLogger(const std::string &logger_name, Level level);

    /**
     * Set sink of logger with name {@param logger_name} to level of his group.
     * Level will be marked as inherited.
     * @returns true if success
     */
    bool resetLevelOfLogger(const std::string &logger_name);

   private:
    /**
     * @returns loggers (with creating that if it isn't exists yet) with
     * name {@param logger_name}, group with name {@param group_name}.
     * Set sink and level to {@param sink_name} and {@param level} if that
     * provided or inherits from the group elsewise
     */
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        const std::optional<std::string> &sink_name,
        const std::optional<Level> &level);

    /**
     * Set parent group of group {@param group} to {@param parent} if it
     * provided and reset elsewise
     */
    void setParentOfGroup(const std::shared_ptr<Group> &group,
                          const std::shared_ptr<Group> &parent);

    /**
     * Set sink of group {@param group} to {@param sink} if it provided and
     * reset elsewise
     */
    void setSinkOfGroup(const std::shared_ptr<Group> &group,
                        std::optional<std::shared_ptr<Sink>> sink);

    /**
     * Set level of group {@param group} to {@param level} if it provided and
     * reset elsewise
     */
    void setLevelOfGroup(const std::shared_ptr<Group> &group,
                         std::optional<Level> level);

    /**
     * Set sink of logger {@param logger} to {@param sink} if it provided and
     * reset elsewise
     */
    static void setSinkOfLogger(const std::shared_ptr<Logger> &logger,
                                std::optional<std::shared_ptr<Sink>> sink);

    /**
     * Set level of logger {@param logger} to {@param level} if it provided and
     * reset elsewise
     */
    static void setLevelOfLogger(const std::shared_ptr<Logger> &logger,
                                 std::optional<Level> level);

    std::shared_ptr<Configurator> configurator_;
    bool is_configured_ = false;
    std::recursive_mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<Logger>> loggers_;
    std::unordered_map<std::string, std::shared_ptr<Sink>> sinks_;
    std::unordered_map<std::string, std::shared_ptr<Group>> groups_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGINGSYSTEM
