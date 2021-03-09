/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/injector.hpp>

#include "logging_object.hpp"

enum ConfiguratorType { Fallback, Customized, YamlByPath, YamlByContent };

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
  - name: main
    sink: console
    level: trace
  - name: azaza
  )"));
  return cfg;
}

int main() {
  ConfiguratorType cfg_type = ConfiguratorType::YamlByContent;

  auto injector = soralog::injector::makeInjector(

      // Replace fallback configurator by ConfiguratorFromYAML
      boost::di::bind<soralog::Configurator>.to([cfg_type](const auto &i) {
        return cfg_type == ConfiguratorType::YamlByContent
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

  auto &object = injector.create<LoggingObject &>();

  object.method();

  main_log->info("Finish");

  return 0;
}
