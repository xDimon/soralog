/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/injector.hpp>

#include "logging_object.hpp"

enum ConfiguratorType {
  Fallback,
  Customized,
  YamlByPath,
  YamlByContent,
  Cascade
};

template <typename Injector>
std::shared_ptr<soralog::Configurator> get_customized_configurator(
    const Injector &injector) {
  static auto cfg = std::make_shared<soralog::FallbackConfigurator>();
  cfg->setLevel(soralog::Level::TRACE);
  cfg->withColor(true);
  return cfg;
}

template <typename Injector>
std::shared_ptr<soralog::Configurator> get_yaml_configurator_from_file(
    const Injector &injector) {
  static auto cfg = std::make_shared<soralog::ConfiguratorFromYAML>(
      std::filesystem::path("../../../example/01-simple/logger.yml"));
  return cfg;
}

template <typename Injector>
std::shared_ptr<soralog::Configurator> get_yaml_configurator_by_content(
    const Injector &injector) {
  static auto cfg =
      std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main_
    sink: console
    level: trace
  - name: azaza
  )"));
  return cfg;
}

template <typename Injector>
std::shared_ptr<soralog::Configurator> get_cascade_configurator(
    const Injector &injector) {
  auto prev = std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
groups:
  - name: main
    level: info
    children:
      - name: first-1
        children:
          - name: second-1-1
          - name: second-1-2
            children:
              - name: third-1-2-1
          - name: second-1-3
      - name: first-2
        children:
          - name: second-2-1
          - name: second-2-2
      - name: first-3
  )"));

  static auto cfg = std::make_shared<soralog::ConfiguratorFromYAML>(
      std::move(prev), std::string(R"(
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: trace
  )"));
  return cfg;
}

int main() {
  ConfiguratorType cfg_type = ConfiguratorType::Cascade;

  auto injector = soralog::injector::makeInjector(

      // Replace fallback configurator by ConfiguratorFromYAML
      boost::di::bind<soralog::Configurator>.to([cfg_type](const auto &i) {
        return cfg_type == ConfiguratorType::Cascade
            ? get_cascade_configurator(i)
            : cfg_type == ConfiguratorType::YamlByContent
                ? get_yaml_configurator_by_content(i)
                : cfg_type == ConfiguratorType::YamlByPath
                    ? get_yaml_configurator_from_file(i)
                    : cfg_type == ConfiguratorType::Customized
                        ? get_customized_configurator(i)
                        : std::make_shared<soralog::FallbackConfigurator>();
      })[boost::di::override]

  );

  auto &log_system = injector.create<soralog::LoggerSystem &>();

  auto r = log_system.configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  auto main_log =
      injector.create<soralog::LoggerFactory &>().getLogger("main", "*");

  main_log->info("Start");

  auto lambda = [](const auto &tag) {
    std::cout << "CALCULATED AT LOGGING: " << tag << std::endl;
    return "message";
  };

  main_log->debug("Debug: {}", lambda("logger: debug for trace level"));
  SL_DEBUG(main_log, "Debug: {}", lambda("macro: debug for trace level"));

  main_log->setLevel(soralog::Level::INFO);

  main_log->trace("Debug: {}", lambda("logger: debug for info level"));
  SL_DEBUG(main_log, "Debug: {}", lambda("macro: debug for info level"));

  auto &object = injector.create<LoggingObject &>();

  object.method();

  main_log->info("Finish");

  return 0;
}
