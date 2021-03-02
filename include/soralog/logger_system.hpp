/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_LOGGERMANAGER
#define SORALOG_LOGGERMANAGER

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

  class LoggerSystem final : public LoggerFactory {
   public:
    LoggerSystem() = delete;
    LoggerSystem(const LoggerSystem &) = delete;
    LoggerSystem &operator=(LoggerSystem const &) = delete;
    ~LoggerSystem() override = default;
    LoggerSystem(LoggerSystem &&tmp) noexcept = delete;
    LoggerSystem &operator=(LoggerSystem &&tmp) noexcept = delete;

    explicit LoggerSystem(std::unique_ptr<Configurator> configurator)
        : configurator_(std::move(configurator)){};

    Configurator::Result configure() {
      if (not is_configured_) {
        is_configured_ = true;
        return configurator_->applyOn(*this);
      }
      return {};
    }

    [[nodiscard]] std::shared_ptr<Logger> getLogger(
        std::string logger_name, const std::string &group_name,
        const std::optional<std::string> &sink_name,
        const std::optional<Level> &level) override;

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

    void setParentForGroup(const std::string &group_name,
                           const std::string &parent);
    void setSinkForGroup(const std::string &group_name,
                         const std::string &sink_name);
    void resetSinkForGroup(const std::string &group_name);
    void setLevelForGroup(const std::string &group_name, Level level);
    void resetLevelForGroup(const std::string &group_name);

    void setGroupForLogger(const std::string &logger_name,
                           const std::string &group_name);
    void setSinkForLogger(const std::string &logger_name,
                          const std::string &sink_name);
    void resetSinkForLogger(const std::string &logger_name);
    void setLevelForLogger(const std::string &logger_name, Level level);
    void resetLevelForLogger(const std::string &logger_name);

   private:
    void setParentForGroup(std::shared_ptr<Group> group,
                           std::shared_ptr<Group> parent);
    void setSinkForGroup(std::shared_ptr<Group> group,
                         std::optional<std::shared_ptr<Sink>> sink);
    void setLevelForGroup(std::shared_ptr<Group> group,
                          std::optional<Level> level);

    void setSinkForLogger(std::shared_ptr<Logger> logger,
                          std::optional<std::shared_ptr<Sink>> sink);
    void setLevelForLogger(std::shared_ptr<Logger> logger,
                           std::optional<Level> level);

    std::unique_ptr<Configurator> configurator_;
    bool is_configured_ = false;
    std::map<std::string, std::weak_ptr<Logger>> loggers_;
    std::map<std::string, std::shared_ptr<Sink>> sinks_;
    std::map<std::string, std::shared_ptr<Group>> groups_;
  };

}  // namespace soralog

#endif  // SORALOG_LOGGERMANAGER
