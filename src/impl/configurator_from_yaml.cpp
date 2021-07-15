/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>

#include <iostream>
#include <memory>
#include <string>

#include <soralog/group.hpp>
#include <soralog/level.hpp>

#include <soralog/impl/sink_to_console.hpp>
#include <soralog/impl/sink_to_file.hpp>
#include <soralog/impl/sink_to_nowhere.hpp>

namespace soralog {

  namespace {

#if defined(WITHOUT_DEBUG_LOG_LEVEL) and not defined(WITHOUT_TRACE_LOG_LEVEL)
#warning "Trace log level have switched off, because bebug log level is off"
#undef WITHOUT_DEBUG_LOG_LEVEL
#endif

    constexpr bool debug_level_disable =
#ifdef WITHOUT_DEBUG_LOG_LEVEL
        true;
#else
        false;
#endif

    constexpr bool trace_level_disabled =
#ifdef WITHOUT_TRACE_LOG_LEVEL
        true;
#else
        false;
#endif

    template <typename>
    inline constexpr bool always_false_v = false;
  }  // namespace

  Configurator::Result ConfiguratorFromYAML::applyOn(
      LoggingSystem &system) const {
    return Applicator(system, config_, previous_).run();
  }

  ConfiguratorFromYAML::Result ConfiguratorFromYAML::Applicator::run() && {
    ConfiguratorFromYAML::Result result;

    if (previous_ != nullptr) {
      result = previous_->applyOn(system_);
    }

    YAML::Node node;

    std::visit(
        [&](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::filesystem::path>) {
            try {
              node = YAML::LoadFile(arg);
            } catch (const std::exception &exception) {
              errors_ << "E: Can't parse file `"
                      << std::filesystem::canonical(arg).string()
                      << "': " << exception.what() << "\n";
              has_error_ = true;
            }

          } else if constexpr (std::is_same_v<T, std::string>) {
            try {
              node = YAML::Load(arg);
            } catch (const std::exception &exception) {
              errors_ << "E: Can't parse content: " << exception.what() << "\n";
              has_error_ = true;
            }

          } else {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
          }
        },
        config_);

    if (not has_error_) {
      parse(node);
    }

    result.has_error = result.has_error || has_error_;
    result.has_warning = result.has_warning || has_warning_;
    result.message += (has_error_ or has_warning_)
        ? ("I: Some problems are found in config:\n" + errors_.str())
        : "";
    return result;
  }

  void ConfiguratorFromYAML::Applicator::parse(const YAML::Node &node) {
    if (not node.IsMap()) {
      errors_ << "E: Config is not YAML map\n";
      has_error_ = true;
      return;
    }

    auto sinks = node["sinks"];

    auto groups = node["groups"];
    if (not groups.IsDefined()) {
      errors_ << "E: Groups are undefined\n";
      has_error_ = true;
    }

    for (const auto &it : node) {
      auto key = it.first.as<std::string>();
      if (key == "sinks")
        continue;
      if (key == "groups")
        continue;
      errors_ << "W: Unknown property: " << key << "\n";
      has_warning_ = true;
    }

    if (sinks.IsDefined()) {
      parseSinks(sinks);
    }

    if (groups.IsDefined()) {
      parseGroups(groups, {});
    }
  }

  void ConfiguratorFromYAML::Applicator::parseSinks(const YAML::Node &sinks) {
    if (sinks.IsNull()) {
      errors_ << "E: Sinks list is empty\n";
      has_error_ = true;
      return;
    }

    if (not sinks.IsSequence()) {
      errors_ << "E: Sinks is not a YAML sequence\n";
      has_error_ = true;
      return;
    }

    for (auto i = 0; i < sinks.size(); ++i) {
      auto sink = sinks[i];
      if (not sink.IsMap()) {
        errors_ << "W: Element #" << i << " of 'sinks' is not a YAML map\n";
        continue;
      }
      parseSink(i, sink);
    }
  }

  void ConfiguratorFromYAML::Applicator::parseSink(int number,
                                                   const YAML::Node &sink) {
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
      fail = true;
      errors_ << "E: Not found 'type' of sink node #" << number << "\n";
      has_error_ = true;
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
    auto type = type_node.as<std::string>();

    if (name == "*") {
      errors_ << "E: Sink name '*' is reserved; "
                 "Try to use some other else\n";
      has_error_ = true;
      return;
    }

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

  void ConfiguratorFromYAML::Applicator::parseSinkToConsole(
      const std::string &name, const YAML::Node &sink_node) {
    bool color = false;
    Sink::ThreadInfoType thread_info_type = Sink::ThreadInfoType::NONE;
    std::optional<size_t> capacity;
    std::optional<size_t> buffer_size;
    std::optional<size_t> latency;

    auto color_node = sink_node["color"];
    if (color_node.IsDefined()) {
      if (not color_node.IsScalar()) {
        errors_ << "W: Property 'color' of sink node is not true or false\n";
        has_warning_ = true;
      } else {
        color = color_node.as<bool>();
      }
    }

    auto thread_node = sink_node["thread"];
    if (thread_node.IsDefined()) {
      if (not thread_node.IsScalar()) {
        errors_ << "W: Property 'thread' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto thread_str = thread_node.as<std::string>();
        if (thread_str == "name") {
          thread_info_type = Sink::ThreadInfoType::NAME;
        } else if (thread_str == "id") {
          thread_info_type = Sink::ThreadInfoType::ID;
        } else if (thread_str != "none") {
          errors_ << "W: Wrong property 'thread' value of sink '" << name
                  << "': " << thread_str << "\n";
          has_warning_ = true;
        }
      }
    }

    auto capacity_node = sink_node["capacity"];
    if (capacity_node.IsDefined()) {
      if (not capacity_node.IsScalar()) {
        errors_ << "W: Property 'capacity' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto capacity_int = capacity_node.as<int>();
        if (capacity_int >= 4) {
          capacity.emplace(capacity_int);
        } else {
          errors_ << "W: Wrong property 'capacity' value of sink '" << name
                  << "': " << capacity_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    auto buffer_node = sink_node["buffer"];
    if (buffer_node.IsDefined()) {
      if (not buffer_node.IsScalar()) {
        errors_ << "W: Property 'buffer' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto buffer_int = buffer_node.as<int>();
        if (buffer_int >= sizeof(Event) * 4) {
          buffer_size.emplace(buffer_int);
        } else {
          errors_ << "W: Wrong property 'buffer' value of sink '" << name
                  << "': " << buffer_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        errors_ << "W: Property 'latency' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          errors_ << "W: Wrong value of property 'latency' value of sink '"
                  << name << "': " << latency_node.as<std::string>() << "\n";
          has_warning_ = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      auto val = it.second;

      if (key == "name")
        continue;
      if (key == "type")
        continue;
      if (key == "color")
        continue;
      if (key == "thread")
        continue;
      if (key == "capacity")
        continue;
      if (key == "buffer")
        continue;
      if (key == "latency")
        continue;
      errors_ << "W: Unknown property of sink '" << name
              << "' with type 'console': " << key << "\n";
      has_warning_ = true;
    }

    if (system_.getSink(name)) {
      errors_ << "W: Already exists sink with name '" << name
              << "'; Previous version will be overridden\n";
      has_warning_ = true;
    }

    system_.makeSink<SinkToConsole>(name, color, thread_info_type, capacity,
                                    buffer_size, latency);
  }

  void ConfiguratorFromYAML::Applicator::parseSinkToFile(
      const std::string &name, const YAML::Node &sink_node) {
    bool fail = false;
    Sink::ThreadInfoType thread_info_type = Sink::ThreadInfoType::NONE;
    std::optional<size_t> capacity;
    std::optional<size_t> buffer_size;
    std::optional<size_t> latency;

    auto path_node = sink_node["path"];
    if (not path_node.IsDefined()) {
      fail = true;
      errors_ << "E: Not found 'path' of sink '" << name << "'\n";
      has_error_ = true;
    } else if (not path_node.IsScalar()) {
      fail = true;
      errors_ << "E: Property 'path' of sink '" << name << "' is not scalar\n";
      has_error_ = true;
    }

    auto thread_node = sink_node["thread"];
    if (thread_node.IsDefined()) {
      if (not thread_node.IsScalar()) {
        errors_ << "W: Property 'thread' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto thread_str = thread_node.as<std::string>();
        if (thread_str == "name") {
          thread_info_type = Sink::ThreadInfoType::NAME;
        } else if (thread_str == "id") {
          thread_info_type = Sink::ThreadInfoType::ID;
        } else if (thread_str != "none") {
          errors_ << "W: Wrong property 'thread' value of sink '" << name
                  << "': " << thread_str << "\n";
          has_warning_ = true;
        }
      }
    }

    auto capacity_node = sink_node["capacity"];
    if (capacity_node.IsDefined()) {
      if (not capacity_node.IsScalar()) {
        errors_ << "W: Property 'capacity' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto capacity_int = capacity_node.as<int>();
        if (capacity_int >= 4) {
          capacity.emplace(capacity_int);
        } else {
          errors_ << "W: Wrong property 'capacity' value of sink '" << name
                  << "': " << capacity_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    auto buffer_node = sink_node["buffer"];
    if (buffer_node.IsDefined()) {
      if (not buffer_node.IsScalar()) {
        errors_ << "W: Property 'buffer' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto buffer_int = buffer_node.as<int>();
        if (buffer_int >= sizeof(Event) * 4) {
          buffer_size.emplace(buffer_int);
        } else {
          errors_ << "W: Wrong property 'buffer' value of sink '" << name
                  << "': " << buffer_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        errors_ << "W: Property 'latency' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          errors_ << "W: Wrong value of property 'latency' value of sink '"
                  << name << "': " << latency_node.as<std::string>() << "\n";
          has_warning_ = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (key == "name")
        continue;
      if (key == "type")
        continue;
      if (key == "path")
        continue;
      if (key == "thread")
        continue;
      if (key == "capacity")
        continue;
      if (key == "buffer")
        continue;
      if (key == "latency")
        continue;
      errors_ << "W: Unknown property of sink '" << name << "': " << key
              << "\n";
      has_warning_ = true;
    }

    if (fail) {
      return;
    }

    auto path = path_node.as<std::string>();

    if (system_.getSink(name)) {
      errors_ << "W: Already exists sink with name '" << name
              << "'; Previous version will be overridden\n";
      has_warning_ = true;
    }

    system_.makeSink<SinkToFile>(name, path, thread_info_type, capacity,
                                 buffer_size, latency);
  }

  void ConfiguratorFromYAML::Applicator::parseGroups(
      const YAML::Node &groups, const std::optional<std::string> &parent) {
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

  void ConfiguratorFromYAML::Applicator::parseGroup(
      int number, const YAML::Node &group_node,
      const std::optional<std::string> &parent) {
    bool fail = false;

    bool is_fallback = false;

    auto name_node = group_node["name"];
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

    auto fallback_node = group_node["is_fallback"];
    if (fallback_node.IsDefined()) {
      if (not fallback_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'is_fallback' of group " << tmp_name
                << " is not scalar\n";
        has_error_ = true;
      } else {
        is_fallback = fallback_node.as<bool>();
      }
    }

    std::optional<std::string> sink{};
    auto sink_node = group_node["sink"];
    if (sink_node.IsDefined()) {
      if (not sink_node.IsScalar()) {
        fail = true;
        errors_ << "E: Property 'sink' of group " << tmp_name
                << " is not scalar\n";
        has_error_ = true;
      } else {
        sink.emplace(sink_node.as<std::string>());
        if (not system_.getSink(sink.value())) {
          fail = true;
          errors_ << "E: Sink '" << *sink << "' of group " << tmp_name
                  << " is undefined\n";
          has_error_ = true;
        }
      }
    } else if (not parent) {
      sink.emplace("*");
    }

    std::optional<std::string> level_string{};
    auto level_node = group_node["level"];
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

    auto children_node = group_node["children"];
    if (children_node.IsDefined()) {
      if (not children_node.IsNull() and not children_node.IsSequence()) {
        fail = true;
        errors_ << "E: Property 'children' of group " << tmp_name
                << " is not sequence\n";
        has_error_ = true;
      }
    }

    for (const auto &it : group_node) {
      auto key = it.first.as<std::string>();

      if (key == "name")
        continue;
      if (key == "is_fallback")
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
      } else if (level_string == "critical" || level_string == "crit") {
        level.emplace(Level::CRITICAL);
      } else if (level_string == "error") {
        level.emplace(Level::ERROR);
      } else if (level_string == "warning" || level_string == "warn") {
        level.emplace(Level::WARN);
      } else if (level_string == "info") {
        level.emplace(Level::INFO);
      } else if (level_string == "verbose") {
        level.emplace(Level::VERBOSE);
      } else if (level_string == "debug" || level_string == "deb") {
        level.emplace(Level::DEBUG);
        if constexpr (debug_level_disable) {
          errors_ << "W: Level 'trace' in group " << tmp_name
                  << " woun't work: it has disabled with compile option"
                  << "\n";
          has_warning_ = true;
        }
      } else if (level_string == "trace") {
        level.emplace(Level::TRACE);
        if constexpr (trace_level_disabled) {
          errors_ << "W: Level 'trace' in group " << tmp_name
                  << " woun't work: it has disabled with compile option"
                  << "\n";
          has_warning_ = true;
        }
      } else {
        errors_ << "E: Invalid level in group " << tmp_name << ": "
                << *level_string << "\n";
        has_error_ = true;
      }
    }

    if (fail) {
      errors_ << "W: There are probably more bugs in the group " << tmp_name
              << "; Fix the existing ones first.\n";
      has_warning_ = true;
      return;
    }

    auto name = name_node.as<std::string>();

    if (name == "*") {
      errors_ << "E: Group name '*' is reserved; "
                 "Try to use some other else\n";
      has_error_ = true;
      return;
    }

    if (system_.getGroup(name)) {
      if (parent.has_value()) {
        system_.setParentOfGroup(name, parent.value());
      }
      if (sink.has_value()) {
        system_.setSinkOfGroup(name, sink.value());
      }
      if (level.has_value()) {
        system_.setLevelOfGroup(name, level.value());
      }
    } else {
      system_.makeGroup(name, parent, sink, level);
    }

    if (is_fallback) {
      system_.setFallbackGroup(name);
    }

    if (children_node.IsDefined() and children_node.IsSequence()) {
      parseGroups(children_node, name);
    }
  }

}  // namespace soralog
