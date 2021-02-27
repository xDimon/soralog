/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_CONFIG
#define SORALOG_CONFIG

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <yaml-cpp/ostream_wrapper.h>
#include <yaml-cpp/yaml.h>

#include <log_levels.hpp>
#include <logger_system.hpp>
#include <sink.hpp>

namespace soralog {

  class Configurator final {
   public:
    Configurator() = delete;
    Configurator(Configurator &&) noexcept = delete;
    Configurator(const Configurator &) = delete;
    virtual ~Configurator() = default;
    Configurator &operator=(Configurator &&) noexcept = delete;
    Configurator &operator=(Configurator const &) = delete;

    explicit Configurator(LoggerSystem &system) : system_(system) {}

    void loadFromFile(const std::filesystem::path &file) {
      YAML::Node node;
      try {
        node = YAML::LoadFile(file);
      } catch (const std::exception &exception) {
        errors_ << "E: Can not parse config file: " << exception.what() << "\n";
        has_error_ = true;
      }

      if (not has_error_) {
        parse(node);
      }

      auto errors = errors_.str();
      if (not errors.empty()) {
        (std::cout << "I: Some problems are found in logger config " << file
                   << ":\n"
                   << errors)
            .flush();
      }
    }

    void parse(const YAML::Node &node) {
      if (not node.IsMap()) {
        errors_ << "E: Root node is not map\n";
        has_error_ = true;
        return;
      }

      auto sinks = node["sinks"];
      if (not sinks.IsDefined()) {
        errors_ << "E: Undefined 'sinks' in root node\n";
        has_error_ = true;
      }

      auto groups = node["groups"];
      if (not groups.IsDefined()) {
        errors_ << "E: Undefined 'groups' in root node\n";
        has_error_ = true;
      }

      for (auto it : node) {
        auto key = it.first.as<std::string>();
        if (key == "sinks")
          continue;
        if (key == "groups")
          continue;
        errors_ << "W: Unknown property in root node: " << key << "\n";
        has_warning_ = true;
      }

      if (sinks.IsDefined()) {
        parseSinks(sinks);
      }

      if (groups.IsDefined()) {
        parseGroups(groups, {});
      }
    }

    void parseSinks(const YAML::Node &sinks) {
      if (sinks.IsNull()) {
        errors_ << "E: Node 'sinks' is empty\n";
        has_error_ = true;
        return;
      }

      if (not sinks.IsSequence()) {
        errors_ << "E: Node 'sinks' is not a sequence\n";
        has_error_ = true;
        return;
      }

      for (auto i = 0; i < sinks.size(); ++i) {
        auto sink = sinks[i];
        if (not sink.IsMap()) {
          errors_ << "W: Element #" << i << " of 'sinks' is not a map\n";
          continue;
        }
        parseSink(i, sink);
      }
    }

    void parseSink(int number, const YAML::Node &sink) {
      bool fail = false;

      auto name_node = sink["name"];
      if (not name_node.IsDefined()) {
        errors_ << "E: Not found 'name' of sink node #" << number << "\n";
        fail = true;
      } else if (not name_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'name' of sink node #" << number
                << " is not scalar\n";
        has_error_ = true;
      }

      auto type_node = sink["type"];
      if (not type_node.IsDefined()) {
        errors_ << "E: Not found 'type' of sink node #" << number << "\n";
        fail = true;
      } else if (not type_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'type' of sink node #" << number
                << "is not scalar\n";
        has_error_ = true;
      }

      if (fail) {
        return;
      }

      auto name = name_node.as<std::string>();
      auto type = name_node.as<std::string>();

      if (type == "console") {
        parseSinkToConsole(name, sink);
      } else if (type == "file") {
        parseSinkToFile(name, sink);
      } else {
        errors_ << "E: Unknown 'type' of sink node '" << name << "': " << type
                << "\n";
        has_error_ = true;
      }
    }

    void parseSinkToConsole(const std::string &name,
                            const YAML::Node &sink_node) {
      bool color = true;

      auto color_node = sink_node["color"];
      if (color_node.IsDefined()) {
        if (not color_node.IsScalar()) {
          errors_ << "W: Property 'color' of sink node is not true or false\n";
          has_warning_ = true;
        } else {
          color = color_node.as<bool>();
        }
      }

      for (auto it : sink_node) {
        auto key = it.first.as<std::string>();
        auto val = it.second;

        if (key == "name")
          continue;
        if (key == "type")
          continue;
        if (key == "color")
          continue;
        errors_ << "W: Unknown property of sink '" << name
                << "' with type 'console': " << key << "\n";
        has_warning_ = true;
      }

      if (system_.getSink(name)) {
        errors_ << "E: Already exists sink with name '" << name << "'\n";
        has_error_ = true;
        return;
      }

      system_.makeSink<SinkToConsole>(name, color);
    }

    void parseSinkToFile(const std::string &name, const YAML::Node &sink) {
      bool fail = false;

      auto dir_node = sink["directory"];
      if (not dir_node.IsDefined()) {
        fail = true;
        errors_ << "E: Not found 'directory' of sink '" << name << "'\n";
        has_error_ = true;
      } else if (not dir_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'directory' of sink '" << name
                << "' is not scalar\n";
        has_error_ = true;
      }

      auto file_node = sink["filename"];
      if (not file_node.IsDefined()) {
        fail = true;
        errors_ << "E: Not found 'filename' of sink '" << name << "'\n";
        has_error_ = true;
      } else if (not file_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'filename' of sink '" << name
                << "' is not scalar\n";
        has_error_ = true;
      }

      for (auto it : sink) {
        auto key = it.first.as<std::string>();
        if (key == "name")
          continue;
        if (key == "type")
          continue;
        if (key == "directory")
          continue;
        if (key == "filename")
          continue;
        errors_ << "W: Unknown property of sink '" << name << "': " << key
                << "\n";
        has_warning_ = true;
      }

      if (fail) {
        return;
      }

      auto directory = dir_node.as<std::string>();
      auto filename = file_node.as<std::string>();

      if (system_.getSink(name)) {
        errors_ << "E: Already exists sink with name '" << name << "'\n";
        has_error_ = true;
        return;
      }

      system_.makeSink<SinkToFile>(name, directory, filename);
    }

    void parseGroups(const YAML::Node &groups,
                     const std::optional<std::string> &parent) {
      if (groups.IsNull()) {
        errors_ << "E: Node 'groups' is empty\n";
        has_error_ = true;
        return;
      }

      if (not groups.IsSequence()) {
        errors_ << "E: Node 'groups' is not a sequence\n";
        has_error_ = true;
        return;
      }

      for (auto i = 0; i < groups.size(); ++i) {
        auto group = groups[i];
        if (not group.IsMap()) {
          errors_ << "E: Element #" << i << " of 'groups' is not a map\n";
          has_error_ = true;
          continue;
        }
        parseGroup(i, group, parent);
      }
    }

    void parseGroup(int number, const YAML::Node &group,
                    const std::optional<std::string> &parent) {
      bool fail = false;

      auto name_node = group["name"];
      std::string tmp_name = "node #" + std::to_string(number);
      if (not name_node.IsDefined()) {
        fail = true;
        errors_ << "W: Not found 'name' of group " << tmp_name << "\n";
        has_error_ = true;
      } else if (not name_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'name' of group " << tmp_name
                << " is not scalar\n";
        has_error_ = true;
      } else {
        tmp_name = "'" + name_node.as<std::string>() + "'";
      }

      std::optional<std::string> sink{};
      auto sink_node = group["sink"];
      if (sink_node.IsDefined()) {
        if (not sink_node.IsScalar()) {
          fail = true;
          errors_ << "E: Property 'sink' of group " << tmp_name
                  << " is not scalar\n";
          has_error_ = true;
        } else {
          sink.emplace(sink_node.as<std::string>());
        }
      } else if (not parent) {
        fail = true;
        errors_ << "E: Not found 'sink' of root group " << tmp_name << "\n";
        has_error_ = true;
      }

      std::optional<std::string> level_string{};
      auto level_node = group["level"];
      if (level_node.IsDefined()) {
        if (not level_node.IsScalar()) {
          fail = true;
          errors_ << "E: Property 'level' of group " << tmp_name
                  << " is not scalar\n";
          has_error_ = true;
        } else {
          level_string.emplace(level_node.as<std::string>());
        }
      } else if (not parent) {
        fail = true;
        errors_ << "E: Not found 'level' of root group " << tmp_name << "\n";
        has_error_ = true;
      }

      auto children_node = group["children"];
      if (children_node.IsDefined()) {
        if (not children_node.IsNull() and not children_node.IsSequence()) {
          fail = true;
          errors_ << "E: Property 'children' of group " << tmp_name
                  << " is not sequence\n";
          has_error_ = true;
        }
      }

      for (auto it : group) {
        auto key = it.first.as<std::string>();

        if (key == "name")
          continue;
        if (key == "sink")
          continue;
        if (key == "level")
          continue;
        if (key == "children")
          continue;
        errors_ << "W: Unknown property of group " << tmp_name << ": " << key
                << "\n";
        has_warning_ = true;
      }

      if (sink) {
        if (not system_.getSink(*sink)) {
          errors_ << "E: Unknown sink in group " << tmp_name << ": " << *sink
                  << "\n";
          has_error_ = true;
        }
      }

      std::optional<Level> level{};
      if (level_string) {
        if (level_string == "off") {
          level.emplace(Level::OFF);
        } else if (level_string == "critical") {
          level.emplace(Level::CRITICAL);
        } else if (level_string == "error") {
          level.emplace(Level::ERROR);
        } else if (level_string == "info") {
          level.emplace(Level::INFO);
        } else if (level_string == "verbose") {
          level.emplace(Level::VERBOSE);
        } else if (level_string == "debug") {
          level.emplace(Level::DEBUG);
        } else if (level_string == "trace") {
          level.emplace(Level::TRACE);
        } else {
          errors_ << "E: Invalid level in group " << tmp_name << ": "
                  << *level_string << "\n";
          has_error_ = true;
        }
      }

      if (fail) {
        errors_ << "W: There are probably more bugs in the group " << tmp_name
                << ". Fix the existing ones first.\n";
        has_warning_ = true;
        return;
      }

      auto name = name_node.as<std::string>();

      system_.makeGroup(name, parent, sink, level);

      if (children_node.IsDefined() and children_node.IsSequence()) {
        parseGroups(children_node, name);
      }
    }

   private:
    LoggerSystem &system_;
    bool has_warning_ = false;
    bool has_error_ = false;
    std::ostringstream errors_;
  };

}  // namespace soralog

#endif  // SORALOG_CONFIG
