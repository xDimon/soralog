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
     * @brief Destroys the configurator.
     */
    ~ConfiguratorFromYAML() override = default;

    void prepare(LoggingSystem &system, size_t index, Result &result) override;
    void applySinks() const override;
    void applyGroups() const override;
    void cleanup() override;

   private:
    /// YAML configuration, which can be a file path, string, or parsed node.
    std::variant<std::filesystem::path, std::string, YAML::Node> config_;

    class Applicator;
    std::shared_ptr<Applicator> applicator_;

    /**
     * @class Applicator
     * @brief Helper class that parses the YAML config and sets up sinks/groups.
     */
    class Applicator {
     public:
      /**
       * @brief Constructs an applicator to process the YAML configuration.
       * @param system Reference to the logging system.
       * @param index
       * @param result
       * @param config YAML configuration (file path, string, or node).
       */
      Applicator(const YAML::Node &node,
                 LoggingSystem &system,
                 size_t index,
                 Result &result)
          : sys(system), node(node), id(index + 1), res(result) {}

      void parseSinks();
      void parseGroups();

     private:
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
      void parseSink(size_t number, const YAML::Node &sink);

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
      void parseGroup(size_t number,
                      const YAML::Node &group_node,
                      const std::optional<std::string> &parent);

      /// Parsed YAML node.
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      const YAML::Node &node;

      /// Reference to the logging system being configured.
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      LoggingSystem &sys;

      /// Index of configurator (as provided in constructor of LoggingSystem)
      size_t id;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
      Result &res;
    };
  };

}  // namespace soralog
