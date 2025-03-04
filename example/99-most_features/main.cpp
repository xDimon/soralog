/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/impl/fallback_configurator.hpp>
#include <soralog/macro.hpp>
#include <soralog/util.hpp>

#include "logging_object.hpp"

using std::literals::string_literals::operator""s;

/**
 * @enum ConfiguratorType
 * @brief Defines various types of configurators for the logging system.
 */
enum class ConfiguratorType : uint8_t {
  Fallback,       ///< Default fallback configurator.
  Customized,     ///< Custom configurator with predefined settings.
  YamlByPath,     ///< Configurator using YAML file from path.
  YamlByContent,  ///< Configurator using YAML content as a string.
  Cascade         ///< Configurator with cascading YAML configurations.
};

// Global configurators for different configurations
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::shared_ptr<soralog::Configurator> customized_configurator = [] {
  static auto cfg = std::make_shared<soralog::FallbackConfigurator>(
      soralog::Level::TRACE, true);
  return cfg;
}();

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::shared_ptr<soralog::Configurator> yaml_configurator_from_file =
    std::make_shared<soralog::ConfiguratorFromYAML>(
        std::filesystem::path("../../../example/01-simple/logger.yml"));

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::shared_ptr<soralog::Configurator> yaml_configurator_by_content =
    std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main_
    is_fallback: true
    sink: console
    level: trace
  - name: azaza
  )"));

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::shared_ptr<soralog::Configurator> cascade_configurator = [] {
  auto prev = std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
groups:
  - name: 3rd_party
    is_fallback: true
    level: info
    children:
      - name: first-1
        children:
          - name: second-1-1
          - name: second-1-2
            children:
              - name: third-1-2-1
                level: critical
          - name: second-1-3
      - name: first-2
        children:
          - name: second-2-1
          - name: second-2-2
      - name: first-3
  )"));

  return std::make_shared<soralog::ConfiguratorFromYAML>(std::move(prev),
                                                         std::string(R"(
sinks:
  - name: console
    type: console
    color: true
    thread: name
groups:
  - name: example_group
    is_fallback: true
    sink: console
    level: trace
    children:
      - name: 3rd_party
  )"));
}();

/**
 * @brief Entry point of the example application.
 *
 * This example demonstrates how to initialize and use the soralog logging
 * system.
 *
 * The program performs the following steps:
 * 1. Selects a logging configurator.
 * 2. Initializes the logging system.
 * 3. Creates a logger instance and logs messages.
 * 4. Spawns multiple threads that also perform logging.
 * 5. Demonstrates formatted logging using macros and dynamic formats.
 * 6. Joins all threads before exiting.
 *
 * @return Exit status of the application.
 */
int main() {
  ConfiguratorType cfg_type = ConfiguratorType::YamlByPath;

  // Select a configurator based on the chosen type
  // clang-format off
  std::shared_ptr<soralog::Configurator> configurator =
      cfg_type == ConfiguratorType::Cascade
      ? cascade_configurator
      : cfg_type == ConfiguratorType::YamlByContent
          ? yaml_configurator_by_content
          : cfg_type == ConfiguratorType::YamlByPath
              ? yaml_configurator_from_file
              : cfg_type == ConfiguratorType::Customized
                  ? customized_configurator
                  : std::make_shared<soralog::FallbackConfigurator>();
  // clang-format on

  // Initialize logging system
  soralog::LoggingSystem log_system(configurator);

  auto r = log_system.configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << '\n';
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  soralog::util::setThreadName("MainThread");

  // Obtain a logger
  auto main_log = log_system.getLogger("main", "example_group");

  // Example of incorrect logging (misuse of placeholders)
  main_log->info("Bad logging (one arg for two placeholders): {} {}", 1);
  main_log->info("Bad logging (unclosed placeholders): {", 1);

  main_log->info("Start");

  // Example of lazy evaluated logging
  auto lambda = [](const auto &tag) {
    std::cout << "CALCULATED: " << tag << '\n';
    return tag;
  };

  main_log->setLevel(soralog::Level::TRACE);
  main_log->debug("{}", lambda("logger: debug msg for trace level"));
  SL_DEBUG(main_log, "{}", lambda("macro: debug msg for trace level"));

  main_log->setLevel(soralog::Level::INFO);
  main_log->debug("{}", lambda("logger: debug msg for info level"));
  SL_DEBUG(main_log, "{}", lambda("macro: debug msg for info level"));

  // Example of formatted logging with dynamic format strings
  std::string generated_format = "<{}>";
  main_log->debug(generated_format, "works!");

  // Example of invalid format in macros causes error in compile time
  // SL_DEBUG(main_log, "{} {}", lambda("one value for two placeholder"));

  std::vector<std::shared_ptr<std::thread>> threads;

  // Launch multiple logging threads
  for (const auto &name :
       {"SecondThread", "ThirdThread", "FourthThread", "FifthThread"}) {
    threads.emplace_back(std::make_shared<std::thread>(std::thread([&] {
      soralog::util::setThreadName(name);
      LoggingObject object(log_system);
      object.method();
    })));
  }

  // Logging a long message
  main_log->info(
      "Very long message  |.....30->|.....40->|.....50->|.....60->|.....70->|"
      ".....80->|.....90->|....100->|....110->|....120->|....130->|....140->|");

  // Example of formatted logging with a dynamically generated format string
  auto dynamic_format = "Custom made format: {} ==>"s + "<== {}"s;
  main_log->info(dynamic_format, 1, 2);
  SL_INFO_DF(main_log, dynamic_format, 3, 4);

  // Demonstration of logging from an object
  LoggingObject object(log_system);
  object.method();

  // Wait for all threads to finish
  for (auto &thread : threads) {
    thread->join();
  }

  main_log->info("Finish");

  return 0;
}
