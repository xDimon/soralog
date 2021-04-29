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

#include <soralog/logging_system.hpp>

namespace soralog {

  /**
   * @class ConfiguratorFromYAML
   * @brief This configurator for set up Logging System in according with
   * config using YAML format.
   */
  class ConfiguratorFromYAML : public Configurator {
   public:
    /**
     * Uses YAML-file {@param config_path} as source of config
     */
    explicit ConfiguratorFromYAML(std::filesystem::path config_path)
        : config_(std::move(config_path)){};

    /**
     * Uses YAML-content {@param config_content} as source of config
     */
    explicit ConfiguratorFromYAML(std::string config_content)
        : config_(std::move(config_content)){};

    /**
     * Uses YAML-file {@param config_path} as source of config.
     * Firstly applies provided underlying configurator {@param previous}.
     */
    explicit ConfiguratorFromYAML(std::shared_ptr<Configurator> previous,
                                  std::filesystem::path config_path)
        : previous_(std::move(previous)), config_(std::move(config_path)){};

    /**
     * Uses YAML-content {@param config_content} as source of config
     * Firstly applies provided underlying configurator {@param previous}.
     */
    explicit ConfiguratorFromYAML(std::shared_ptr<Configurator> previous,
                                  std::string config_content)
        : previous_(std::move(previous)), config_(std::move(config_content)){};

    ~ConfiguratorFromYAML() override = default;

    Result applyOn(LoggingSystem &system) const override;

   private:
    std::shared_ptr<Configurator> previous_;
    std::variant<std::filesystem::path, std::string> config_;

    /**
     * Helper-class to parse config and create sinks and groups during that
     */
    class Applicator {
     public:
      Applicator(LoggingSystem &system,
                 std::variant<std::filesystem::path, std::string> config,
                 std::shared_ptr<Configurator> previous = {})
          : system_(system),
            previous_(std::move(previous)),
            config_(std::move(config)) {}

      Result run() &&;

     private:
      void parse(const YAML::Node &node);

      void parseSinks(const YAML::Node &sinks);

      void parseSink(int number, const YAML::Node &sink);

      void parseSinkToConsole(const std::string &name,
                              const YAML::Node &sink_node);

      void parseSinkToFile(const std::string &name,
                           const YAML::Node &sink_node);

      void parseGroups(const YAML::Node &groups,
                       const std::optional<std::string> &parent);

      void parseGroup(int number, const YAML::Node &group_node,
                      const std::optional<std::string> &parent);

      LoggingSystem &system_;
      std::shared_ptr<Configurator> previous_ = nullptr;
      std::variant<std::filesystem::path, std::string> config_;
      bool has_warning_ = false;
      bool has_error_ = false;
      std::ostringstream errors_;
    };
  };

}  // namespace soralog

#endif  // SORALOG_CONFIGURATORFROMYAML
