/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <soralog/configurator.hpp>

#include <filesystem>
#include <variant>

#include <yaml-cpp/yaml.h>

#include <soralog/logging_system.hpp>

namespace soralog {

  /**
   * @class ConfiguratorFromYAML
   * @brief Configurator that sets up the logging system using a YAML config.
   *
   * This configurator reads a YAML configuration file or string and applies
   * it to the logging system, setting up sinks and groups accordingly.
   */
  class ConfiguratorFromYAML : public Configurator {
   public:
    /**
     * @brief Constructs a configurator using a YAML file.
     * @param config_path Path to the YAML configuration file.
     */
    explicit ConfiguratorFromYAML(std::filesystem::path config_path)
        : config_(std::move(config_path)) {}

    /**
     * @brief Constructs a configurator using a YAML string.
     * @param config_content YAML configuration content as a string.
     */
    explicit ConfiguratorFromYAML(std::string config_content)
        : config_(std::move(config_content)) {}

    /**
     * @brief Constructs a configurator using a YAML file,
     *        applying a previous configurator first.
     * @param previous Underlying configurator to apply first.
     * @param config_path Path to the YAML configuration file.
     */
    explicit ConfiguratorFromYAML(std::shared_ptr<Configurator> previous,
                                  std::filesystem::path config_path)
        : previous_(std::move(previous)), config_(std::move(config_path)) {}

    /**
     * @brief Constructs a configurator using a YAML string,
     *        applying a previous configurator first.
     * @param previous Underlying configurator to apply first.
     * @param config_content YAML configuration content as a string.
     */
    explicit ConfiguratorFromYAML(std::shared_ptr<Configurator> previous,
                                  std::string config_content)
        : previous_(std::move(previous)), config_(std::move(config_content)) {}

    /**
     * @brief Constructs a configurator using a parsed YAML node,
     *        applying a previous configurator first.
     * @param previous Underlying configurator to apply first.
     * @param config_yaml_node Parsed YAML configuration node.
     */
    explicit ConfiguratorFromYAML(std::shared_ptr<Configurator> previous,
                                  YAML::Node config_yaml_node)
        : previous_(std::move(previous)),
          config_(std::move(config_yaml_node)) {}

    /**
     * @brief Destroys the configurator.
     */
    ~ConfiguratorFromYAML() override = default;

    /**
     * @brief Applies the YAML-based configuration to the logging system.
     * @param system The logging system instance.
     * @return Configuration result, indicating errors or warnings.
     */
    Result applyOn(LoggingSystem &system) const override;

   private:
    /// Optional previous configurator to apply before parsing the YAML config.
    std::shared_ptr<Configurator> previous_;

    /// YAML configuration, which can be a file path, string, or parsed node.
    std::variant<std::filesystem::path, std::string, YAML::Node> config_;

    /**
     * @class Applicator
     * @brief Helper class that parses the YAML config and sets up sinks/groups.
     */
    class Applicator {
     public:
      /**
       * @brief Constructs an applicator to process the YAML configuration.
       * @param system Reference to the logging system.
       * @param config YAML configuration (file path, string, or node).
       * @param previous Optional previous configurator.
       */
      Applicator(
          LoggingSystem &system,
          std::variant<std::filesystem::path, std::string, YAML::Node> config,
          std::shared_ptr<Configurator> previous = {})
          : system_(system),
            previous_(std::move(previous)),
            config_(std::move(config)) {}

      /**
       * @brief Executes the configuration parsing and application process.
       * @return Result object containing status and messages about the process.
       */
      Result run() &&;

     private:
      /**
       * @brief Parses the root YAML node and applies configurations.
       * @param node The root YAML node.
       */
      void parse(const YAML::Node &node);

      /**
       * @brief Parses and returns a log level from a YAML node.
       * @param target Description of the entity being parsed..
       * @param node YAML node containing the level.
       * @return Parsed log level or `std::nullopt` on failure.
       */
      std::optional<Level> parseLevel(const std::string &target,
                                      const YAML::Node &node);

      /**
       * @brief Parses and registers sink configurations.
       * @param sinks YAML node containing sink configurations.
       */
      void parseSinks(const YAML::Node &sinks);

      /**
       * @brief Parses a single sink configuration from a YAML node.
       *
       * This method reads and validates a sink configuration from the provided
       * YAML node, then calls the corresponding method to create the sink
       * based on its type.
       *
       * @param number Index of the sink in the YAML sequence (for error
       * reporting).
       * @param sink YAML node containing the sink configuration.
       */
      void parseSink(int number, const YAML::Node &sink);

      /**
       * @brief Parses a console sink configuration from a YAML node.
       *
       * This method extracts and validates properties for a console sink,
       * handling options such as color output, stream type (stdout/stderr),
       * thread info, buffer settings, and error-handling strategies.
       *
       * @param name Name of the sink being configured.
       * @param sink_node YAML node containing the console sink configuration.
       */
      void parseSinkToConsole(const std::string &name,
                              const YAML::Node &sink_node);

      /**
       * @brief Parses a file sink configuration from a YAML node.
       *
       * This method extracts and validates properties for a file-based log
       * sink, including file path, thread info, buffer settings, latency, and
       * fault handling.
       *
       * @param name Name of the sink being configured.
       * @param sink_node YAML node containing the file sink configuration.
       */
      void parseSinkToFile(const std::string &name,
                           const YAML::Node &sink_node);

      /**
       * @brief Parses a syslog sink configuration from a YAML node.
       *
       * This method extracts and validates properties for a syslog-based log
       * sink, including the syslog identifier, thread info, buffer settings,
       * latency, and fault handling.
       *
       * @param name Name of the sink being configured.
       * @param sink_node YAML node containing the syslog sink configuration.
       */
      void parseSinkToSyslog(const std::string &name,
                             const YAML::Node &sink_node);

      /**
       * @brief Parses a multisink configuration from a YAML node.
       *
       * This method reads and validates a multisink configuration from YAML,
       * extracting the list of underlying sinks and setting the logging level.
       * The multisink aggregates multiple sinks, allowing logs to be forwarded
       * to multiple destinations simultaneously.
       *
       * @param name Name of the multisink being configured.
       * @param sink_node YAML node containing the multisink configuration.
       */
      void parseMultisink(const std::string &name, const YAML::Node &sink_node);

      /**
       * @brief Parses a list of logging groups from a YAML node.
       *
       * This method processes a YAML sequence containing group configurations,
       * validates their structure, and invokes `parseGroup` for each entry.
       *
       * @param groups YAML node containing the list of logging groups.
       * @param parent Optional name of the parent group, if applicable.
       */
      void parseGroups(const YAML::Node &groups,
                       const std::optional<std::string> &parent);

      /**
       * @brief Parses a single logging group from a YAML node.
       *
       * This method processes an individual group configuration, validating its
       * properties and creating the corresponding group in the logging system.
       *
       * @param number The index of the group in the YAML sequence.
       * @param group_node YAML node containing the group definition.
       * @param parent Optional name of the parent group, if applicable.
       */
      void parseGroup(int number,
                      const YAML::Node &group_node,
                      const std::optional<std::string> &parent);

      /// Reference to the logging system being configured.
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      LoggingSystem &system_;

      /// Optional previous configurator applied before parsing YAML.
      std::shared_ptr<Configurator> previous_ = nullptr;

      /// YAML configuration (file path, string, or parsed node).
      std::variant<std::filesystem::path, std::string, YAML::Node> config_;

      /// Flag indicating if a warning occurred during parsing.
      bool has_warning_ = false;

      /// Flag indicating if an error occurred during parsing.
      bool has_error_ = false;

      /// Stream for collecting error messages.
      std::ostringstream errors_;
    };
  };

}  // namespace soralog
