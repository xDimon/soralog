/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CONFIGURATORFROMYAML
#define SORALOG_CONFIGURATORFROMYAML

#include <soralog/configurator.hpp>

#include <filesystem>
#include <variant>

#include <yaml-cpp/yaml.h>

#include <soralog/logger_system.hpp>

namespace soralog {

  class ConfiguratorFromYAML : public Configurator {
   public:
    explicit ConfiguratorFromYAML(std::filesystem::path config_path)
        : config_(std::move(config_path)){};

    explicit ConfiguratorFromYAML(std::string config_content)
        : config_(std::move(config_content)){};

    ~ConfiguratorFromYAML() override = default;

    Result applyOn(LoggerSystem &system) const override;

   private:
    std::variant<std::filesystem::path, std::string> config_;

    class Applicator {
     public:
      Applicator(LoggerSystem &system, std::variant<std::filesystem::path, std::string> config)
          : system_(system), config_(std::move(config)) {}

      Result run() &&;

     private:
      void parse(const YAML::Node &node);

      void parseSinks(const YAML::Node &sinks);

      void parseSink(int number, const YAML::Node &sink);

      void parseSinkToConsole(const std::string &name,
                              const YAML::Node &sink_node);

      void parseSinkToFile(const std::string &name, const YAML::Node &sink);

      void parseGroups(const YAML::Node &groups,
                       const std::optional<std::string> &parent);

      void parseGroup(int number, const YAML::Node &group,
                      const std::optional<std::string> &parent);

      LoggerSystem &system_;
      std::variant<std::filesystem::path, std::string> config_;
      bool has_warning_ = false;
      bool has_error_ = false;
      std::ostringstream errors_;
    };
  };

}  // namespace soralog

#endif  // SORALOG_CONFIGURATORFROMYAML
