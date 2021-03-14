/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGINGSYSTEM
#define SORALOG_LOGGINGSYSTEM

#include <soralog/logger_factory.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>

#include <soralog/configurator.hpp>
#include <soralog/sink.hpp>

namespace soralog {

  class Group;
  class Logger;

  class LoggingSystem final : public LoggerFactory {
   public:
    LoggingSystem() = delete;
    LoggingSystem(const LoggingSystem &) = delete;
    LoggingSystem &operator=(LoggingSystem const &) = delete;
    ~LoggingSystem() override = default;
    LoggingSystem(LoggingSystem &&tmp) noexcept = delete;
    LoggingSystem &operator=(LoggingSystem &&tmp) noexcept = delete;

    explicit LoggingSystem(std::shared_ptr<Configurator> configurator)
        : configurator_(std::move(configurator)){};

    [[nodiscard]] Configurator::Result configure();

    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name) override {
      return getLogger(std::move(logger_name), group_name, std::nullopt,
                       std::nullopt);
    }

    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        Level level) override {
      return getLogger(std::move(logger_name), group_name, std::nullopt,
                       std::make_optional(level));
    }

    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name) override {
      return getLogger(std::move(logger_name), group_name,
                       std::make_optional(std::move(sink_name)), std::nullopt);
    }

    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        std::string sink_name, Level level) override {
      return getLogger(std::move(logger_name), group_name,
                       std::make_optional(std::move(sink_name)),
                       std::make_optional(level));
    }

    [[nodiscard]] std::shared_ptr<Sink> getSink(const std::string &name);

    [[nodiscard]] std::shared_ptr<Group> getGroup(const std::string &name);

    template <typename SinkType, typename... Args>
    void makeSink(Args &&... args) {
      auto sink = std::make_shared<SinkType>(std::forward<Args>(args)...);
      sinks_[sink->name()] = std::move(sink);
    }

    void makeGroup(std::string name, const std::optional<std::string> &parent,
                   const std::optional<std::string> &sink,
                   const std::optional<Level> &level);

    bool setParentOfGroup(const std::string &group_name,
                          const std::string &parent);
    bool setSinkOfGroup(const std::string &group_name,
                        const std::string &sink_name);
    bool resetSinkOfGroup(const std::string &group_name);
    bool setLevelOfGroup(const std::string &group_name, Level level);
    bool resetLevelOfGroup(const std::string &group_name);

    bool setGroupOfLogger(const std::string &logger_name,
                          const std::string &group_name);
    bool setSinkOfLogger(const std::string &logger_name,
                         const std::string &sink_name);
    bool resetSinkOfLogger(const std::string &logger_name);
    bool setLevelOfLogger(const std::string &logger_name, Level level);
    bool resetLevelOfLogger(const std::string &logger_name);

   private:
    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        const std::optional<std::string> &sink_name,
        const std::optional<Level> &level);

    void setParentOfGroup(const std::shared_ptr<Group> &group,
                          const std::shared_ptr<Group> &parent);
    void setSinkOfGroup(const std::shared_ptr<Group> &group,
                        std::optional<std::shared_ptr<Sink>> sink);
    void setLevelOfGroup(const std::shared_ptr<Group> &group,
                         std::optional<Level> level);

    static void setSinkOfLogger(const std::shared_ptr<Logger> &logger,
                                std::optional<std::shared_ptr<Sink>> sink);
    static void setLevelOfLogger(const std::shared_ptr<Logger> &logger,
                                 std::optional<Level> level);

    std::shared_ptr<Configurator> configurator_;
    bool is_configured_ = false;
    std::map<std::string, std::weak_ptr<Logger>> loggers_;
    std::map<std::string, std::shared_ptr<Sink>> sinks_;
    std::map<std::string, std::shared_ptr<Group>> groups_;
    std::shared_ptr<Group> fallback_group_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGINGSYSTEM
