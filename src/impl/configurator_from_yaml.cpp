/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/configurator_from_yaml.hpp>

#include <iostream>
#include <memory>
#include <string>

#include <soralog/group.hpp>
#include <soralog/level.hpp>

#include <soralog/impl/multisink.hpp>
#include <soralog/impl/sink_to_console.hpp>
#include <soralog/impl/sink_to_file.hpp>
#include <soralog/impl/sink_to_nowhere.hpp>
#include <soralog/impl/sink_to_syslog.hpp>

namespace soralog {

  namespace {

#if defined(WITHOUT_DEBUG_LOG_LEVEL) and not defined(WITHOUT_TRACE_LOG_LEVEL)
#warning "Trace log level has been switched off because debug log level is off"
#undef WITHOUT_DEBUG_LOG_LEVEL
#endif

    /// Indicates whether debug level logs are disabled at compile time
    constexpr bool debug_level_disable =
#ifdef WITHOUT_DEBUG_LOG_LEVEL
        true;
#else
        false;
#endif

    /// Indicates whether trace level logs are disabled at compile time
    constexpr bool trace_level_disabled =
#ifdef WITHOUT_TRACE_LOG_LEVEL
        true;
#else
        false;
#endif

    /// Helper template for ensuring exhaustive type handling in `std::visit`
    template <typename>
    inline constexpr bool always_false_v = false;
  }  // namespace

  Configurator::Result ConfiguratorFromYAML::applyOn(
      LoggingSystem &system) const {
    return Applicator(system, config_, previous_).run();
  }

  ConfiguratorFromYAML::Result ConfiguratorFromYAML::Applicator::run() && {
    Result result;

    if (previous_ != nullptr) {
      result = previous_->applyOn(system_);
    }

    YAML::Node node;

    std::visit(
        [&](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::filesystem::path>) {
            // Load YAML from a file
            try {
              node = YAML::LoadFile(arg);
            } catch (const std::exception &exception) {
              errors_ << "E: Can't parse file "
                      << std::filesystem::weakly_canonical(arg) << ": "
                      << exception.what() << "\n";
              has_error_ = true;
            }
          } else if constexpr (std::is_same_v<T, std::string>) {
            // Load YAML from a string
            try {
              node = YAML::Load(arg);
            } catch (const std::exception &exception) {
              errors_ << "E: Can't parse content: " << exception.what() << "\n";
              has_error_ = true;
            }
          } else if constexpr (std::is_same_v<T, YAML::Node>) {
            // Use the provided YAML node directly
            node = arg;
          } else {
            static_assert(always_false_v<T>, "Unhandled configuration type!");
          }
        },
        config_);

    if (not has_error_) {
      parse(node);
    }

    result.has_error = result.has_error || has_error_;
    result.has_warning = result.has_warning || has_warning_;
    if (result.has_error or result.has_warning) {
      result.message += "I: Some problems are found during configuring:\n"
                      + errors_.str()
                      + "I: See more details on "
                        "https://github.com/xDimon/soralog/tree/update/"
                        "documentation?tab=readme-ov-file#configuration-file";
    }
    return result;
  }

  void ConfiguratorFromYAML::Applicator::parse(const YAML::Node &node) {
    if (not node.IsMap()) {
      errors_ << "E: Config is not a YAML map\n";
      has_error_ = true;
      return;
    }

    auto sinks = node["sinks"];

    auto groups = node["groups"];

    if (not groups.IsDefined()) {
      errors_ << "E: Groups are undefined\n";
      has_error_ = true;
    }

    // Validate top-level keys
    for (const auto &it : node) {
      auto key = it.first.as<std::string>();
      if (key == "sinks" or key == "groups") {
        continue;
      }
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

  std::optional<Level> ConfiguratorFromYAML::Applicator::parseLevel(
      const std::string &target, const YAML::Node &node) {
    auto level_node = node["level"];
    if (not level_node.IsDefined()) {
      return std::nullopt;
    }
    if (not level_node.IsScalar()) {
      errors_ << "E: Property 'level' of " << target << " is not scalar\n";
      has_error_ = true;
      return std::nullopt;
    }

    auto level_string = level_node.as<std::string>();

    if (level_string == "off") {
      return Level::OFF;
    }
    if (level_string == "critical" || level_string == "crit") {
      return Level::CRITICAL;
    }
    if (level_string == "error") {
      return Level::ERROR;
    }
    if (level_string == "warning" || level_string == "warn") {
      return Level::WARN;
    }
    if (level_string == "info") {
      return Level::INFO;
    }
    if (level_string == "verbose") {
      return Level::VERBOSE;
    }
    if (level_string == "debug" || level_string == "deb") {
      if constexpr (debug_level_disable) {
        errors_ << "W: Level 'debug' in " << target << " won't work: "
                << "it has been disabled with a compile-time option\n";
        has_warning_ = true;
      }
      return Level::DEBUG;
    }
    if (level_string == "trace") {
      if constexpr (trace_level_disabled) {
        errors_ << "W: Level 'trace' in " << target
                << " won't work: "
                   "it has been disabled with a compile-time option\n";
        has_warning_ = true;
      }
      return Level::TRACE;
    }
    errors_ << "E: Invalid level in " << target << ": " << level_string << "\n";
    has_error_ = true;
    return std::nullopt;
  }

  void ConfiguratorFromYAML::Applicator::parseSink(int number,
                                                   const YAML::Node &sink) {
    bool fail = false;

    // Extract and validate the sink name
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

    // Extract and validate the sink type
    auto type_node = sink["type"];
    if (not type_node.IsDefined()) {
      fail = true;
      errors_ << "E: Not found 'type' of sink node #" << number << "\n";
      has_error_ = true;
    } else if (not type_node.IsScalar()) {
      fail = true;
      errors_ << "E: Property 'type' of sink node #" << number
              << " is not scalar\n";
      has_error_ = true;
    }

    if (fail) {
      return;
    }

    auto name = name_node.as<std::string>();
    auto type = type_node.as<std::string>();

    // Ensure the sink name is not reserved
    if (name == "*") {
      errors_ << "E: Sink name '*' is reserved; "
                 "Try to use some other name\n";
      has_error_ = true;
      return;
    }

    // Dispatch the sink creation based on the type
    if (type == "console") {
      parseSinkToConsole(name, sink);
    } else if (type == "file") {
      parseSinkToFile(name, sink);
    } else if (type == "syslog") {
      parseSinkToSyslog(name, sink);
    } else if (type == "multisink") {
      parseMultisink(name, sink);
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
    SinkToConsole::Stream stream_type = SinkToConsole::Stream::STDOUT;
    std::optional<size_t> capacity;
    std::optional<size_t> buffer_size;
    std::optional<size_t> max_message_length;
    std::optional<size_t> latency;
    Sink::AtFaultReactionType at_fault = Sink::AtFaultReactionType::WAIT;

    // Parse 'color' option
    auto color_node = sink_node["color"];
    if (color_node.IsDefined()) {
      if (not color_node.IsScalar()) {
        errors_ << "W: Property 'color' of sink node is not true or false\n";
        has_warning_ = true;
      } else {
        color = color_node.as<bool>();
      }
    }

    // Parse 'stream' option (stdout/stderr)
    auto stream_node = sink_node["stream"];
    if (stream_node.IsDefined()) {
      if (not stream_node.IsScalar()) {
        errors_
            << "W: Property 'stream' of sink node is not stdout or stderr\n";
        has_warning_ = true;
      } else {
        auto stream_str = stream_node.as<std::string>();
        if (stream_str == "stdout") {
          stream_type = SinkToConsole::Stream::STDOUT;
        } else if (stream_str == "stderr") {
          stream_type = SinkToConsole::Stream::STDERR;
        } else {
          errors_
              << "W: Invalid 'stream' value: expected 'stdout' or 'stderr'\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'thread' information type
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
          errors_ << "W: Invalid 'thread' value of sink '" << name
                  << "': " << thread_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse capacity settings
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
          errors_ << "W: Invalid 'capacity' value of sink '" << name
                  << "': " << capacity_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse buffer size settings
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
          errors_ << "W: Invalid 'buffer' value of sink '" << name
                  << "': " << buffer_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse max message length settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        errors_
            << "W: Property 'max_message_length' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int > 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          errors_ << "W: Invalid 'max_message_length' value of sink '" << name
                  << "': " << max_message_length_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse latency settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        errors_ << "W: Property 'latency' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          errors_ << "W: Invalid 'latency' value of sink '" << name
                  << "': " << latency_node.as<std::string>() << "\n";
          has_warning_ = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse fault reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        errors_ << "W: Property 'at_fault' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          errors_ << "W: Invalid 'at_fault' value of sink '" << name
                  << "': " << at_fault_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse logging level
    auto level = parseLevel(fmt::format("sink '{}'", name), sink_node)
                     .value_or(Level::TRACE);

    // Check for unknown properties
    static constexpr std::array known_properties = {"name",
                                                    "type",
                                                    "stream",
                                                    "color",
                                                    "thread",
                                                    "capacity",
                                                    "buffer",
                                                    "max_message_length",
                                                    "latency",
                                                    "at_fault",
                                                    "level"};
    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        errors_ << "W: Unknown property of sink '" << name << "': " << key
                << "\n";
        has_warning_ = true;
      }
    }

    // Check if the sink already exists
    if (system_.getSink(name)) {
      errors_ << "W: Sink with name '" << name << "' already exists; "
              << "overriding previous version\n";
      has_warning_ = true;
    }

    // Create the console sink
    system_.makeSink<SinkToConsole>(name,
                                    level,
                                    stream_type,
                                    color,
                                    thread_info_type,
                                    capacity,
                                    max_message_length,
                                    buffer_size,
                                    latency,
                                    at_fault);
  }

  void ConfiguratorFromYAML::Applicator::parseSinkToFile(
      const std::string &name, const YAML::Node &sink_node) {
    bool fail = false;
    Sink::ThreadInfoType thread_info_type = Sink::ThreadInfoType::NONE;
    std::optional<size_t> capacity;
    std::optional<size_t> buffer_size;
    std::optional<size_t> max_message_length;
    std::optional<size_t> latency;
    Sink::AtFaultReactionType at_fault = Sink::AtFaultReactionType::WAIT;

    // Parse 'path' option (file destination)
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

    // Parse 'thread' information type
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
          errors_ << "W: Invalid 'thread' value of sink '" << name
                  << "': " << thread_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'capacity' settings
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
          errors_ << "W: Invalid 'capacity' value of sink '" << name
                  << "': " << capacity_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'buffer' size settings
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
          errors_ << "W: Invalid 'buffer' value of sink '" << name
                  << "': " << buffer_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'max_message_length' settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        errors_
            << "W: Property 'max_message_length' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int >= 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          errors_ << "W: Invalid 'max_message_length' value of sink '" << name
                  << "': " << max_message_length_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'latency' settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        errors_ << "W: Property 'latency' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          errors_ << "W: Invalid 'latency' value of sink '" << name
                  << "': " << latency_node.as<std::string>() << "\n";
          has_warning_ = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse 'at_fault' reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        errors_ << "W: Property 'at_fault' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          errors_ << "W: Invalid 'at_fault' value of sink '" << name
                  << "': " << at_fault_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse logging level
    auto level = parseLevel(fmt::format("sink '{}'", name), sink_node)
                     .value_or(Level::TRACE);

    // Check for unknown properties
    static constexpr std::array known_properties = {"name",
                                                    "type",
                                                    "path",
                                                    "thread",
                                                    "capacity",
                                                    "buffer",
                                                    "max_message_length",
                                                    "latency",
                                                    "at_fault",
                                                    "level"};
    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        errors_ << "W: Unknown property of sink '" << name << "': " << key
                << "\n";
        has_warning_ = true;
      }
    }

    if (fail) {
      return;
    }

    auto path = path_node.as<std::string>();

    // Check if the sink already exists
    if (system_.getSink(name)) {
      errors_ << "W: Sink with name '" << name << "' already exists; "
              << "overriding previous version\n";
      has_warning_ = true;
    }

    // Create the file sink
    system_.makeSink<SinkToFile>(name,
                                 level,
                                 path,
                                 thread_info_type,
                                 capacity,
                                 max_message_length,
                                 buffer_size,
                                 latency,
                                 at_fault);
  }

  void ConfiguratorFromYAML::Applicator::parseSinkToSyslog(
      const std::string &name, const YAML::Node &sink_node) {
    bool fail = false;
    Sink::ThreadInfoType thread_info_type = Sink::ThreadInfoType::NONE;
    std::optional<size_t> capacity;
    std::optional<size_t> buffer_size;
    std::optional<size_t> max_message_length;
    std::optional<size_t> latency;
    Sink::AtFaultReactionType at_fault = Sink::AtFaultReactionType::WAIT;

    // Parse 'ident' (syslog identifier)
    auto ident_node = sink_node["ident"];
    if (not ident_node.IsDefined()) {
      fail = true;
      errors_ << "E: Not found 'ident' of sink '" << name << "'\n";
      has_error_ = true;
    } else if (not ident_node.IsScalar()) {
      fail = true;
      errors_ << "E: Property 'ident' of sink '" << name << "' is not scalar\n";
      has_error_ = true;
    }

    // Parse 'thread' information type
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
          errors_ << "W: Invalid 'thread' value of sink '" << name
                  << "': " << thread_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'capacity' settings
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
          errors_ << "W: Invalid 'capacity' value of sink '" << name
                  << "': " << capacity_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'buffer' size settings
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
          errors_ << "W: Invalid 'buffer' value of sink '" << name
                  << "': " << buffer_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'max_message_length' settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        errors_
            << "W: Property 'max_message_length' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int >= 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          errors_ << "W: Invalid 'max_message_length' value of sink '" << name
                  << "': " << max_message_length_node.as<std::string>() << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse 'latency' settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        errors_ << "W: Property 'latency' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          errors_ << "W: Invalid 'latency' value of sink '" << name
                  << "': " << latency_node.as<std::string>() << "\n";
          has_warning_ = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse 'at_fault' reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        errors_ << "W: Property 'at_fault' of sink node is not scalar\n";
        has_warning_ = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          errors_ << "W: Invalid 'at_fault' value of sink '" << name
                  << "': " << at_fault_str << "\n";
          has_warning_ = true;
        }
      }
    }

    // Parse logging level
    auto level = parseLevel(fmt::format("sink '{}'", name), sink_node)
                     .value_or(Level::TRACE);

    // Check for unknown properties
    static constexpr std::array known_properties = {"name",
                                                    "type",
                                                    "ident",
                                                    "thread",
                                                    "capacity",
                                                    "buffer",
                                                    "max_message_length",
                                                    "latency",
                                                    "at_fault",
                                                    "level"};
    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        errors_ << "W: Unknown property of sink '" << name << "': " << key
                << "\n";
        has_warning_ = true;
      }
    }

    if (fail) {
      return;
    }

    auto ident = ident_node.as<std::string>();

    // Check if the sink already exists
    if (system_.getSink(name)) {
      errors_ << "W: Sink with name '" << name << "' already exists; "
              << "overriding previous version\n";
      has_warning_ = true;
    }

    // Create the syslog sink
    system_.makeSink<SinkToSyslog>(name,
                                   level,
                                   ident,
                                   thread_info_type,
                                   capacity,
                                   max_message_length,
                                   buffer_size,
                                   latency,
                                   at_fault);
  }

  void ConfiguratorFromYAML::Applicator::parseMultisink(
      const std::string &name, const YAML::Node &sink_node) {
    bool fail = false;

    // Retrieve the list of underlying sinks
    auto sinks_node = sink_node["sinks"];
    if (not sinks_node.IsDefined()) {
      fail = true;
      errors_ << "E: Not found 'sinks' of sink '" << name << "'\n";
      has_error_ = true;
    } else if (not sinks_node.IsSequence()) {
      fail = true;
      errors_ << "E: Property 'sinks' of sink '" << name << "' is not a list\n";
      has_error_ = true;
    }

    // Parse the logging level for this multisink
    auto level = parseLevel(fmt::format("sink '{}'", name), sink_node)
                     .value_or(Level::TRACE);

    // Check for unknown properties
    static constexpr std::array known_properties = {
        "name", "type", "sinks", "level"};
    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        errors_ << "W: Unknown property of sink '" << name << "': " << key
                << "\n";
        has_warning_ = true;
      }
    }

    if (fail) {
      return;
    }

    // Extract sink names and resolve them to actual sink objects
    auto sink_names = sinks_node.as<std::vector<std::string>>();

    std::vector<std::shared_ptr<Sink>> sinks;
    for (auto &sink_name : sink_names) {
      auto sink = system_.getSink(sink_name);
      if (not sink) {
        errors_ << "E: Sink '" << sink_name << "' must be defined before sink '"
                << name << "'\n";
        has_warning_ = true;
      } else {
        sinks.emplace_back(std::move(sink));
      }
    }

    // Create the multisink
    system_.makeSink<Multisink>(name, level, std::move(sinks));
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

    // Iterate over the group list and parse each entry
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
      int number,
      const YAML::Node &group_node,
      const std::optional<std::string> &parent) {
    bool fail = false;
    bool is_fallback = false;

    // Extract and validate the group name
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

    // Check if the group is marked as a fallback group
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

    // Extract and validate the sink property
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

    // Extract and validate the level property
    auto level_node = group_node["level"];
    if (not level_node.IsDefined() and not parent) {
      fail = true;
      errors_ << "E: Not found 'level' of root group " << tmp_name << "\n";
      has_error_ = true;
    }
    auto level = parseLevel(fmt::format("group '{}'", tmp_name), group_node);

    // Validate the children property
    auto children_node = group_node["children"];
    if (children_node.IsDefined()) {
      if (not children_node.IsNull() and not children_node.IsSequence()) {
        fail = true;
        errors_ << "E: Property 'children' of group " << tmp_name
                << " is not sequence\n";
        has_error_ = true;
      }
    }

    // Check for unknown properties
    static constexpr std::array known_properties = {
        "name", "is_fallback", "sink", "level", "children"};
    for (const auto &it : sink_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        errors_ << "W: Unknown property of sink '" << tmp_name << "': " << key
                << "\n";
        has_warning_ = true;
      }
    }

    // Validate the sink existence
    if (sink and not system_.getSink(*sink)) {
      errors_ << "E: Unknown sink in group " << tmp_name << ": " << *sink
              << "\n";
      has_error_ = true;
    }

    if (fail) {
      errors_ << "W: There are probably more bugs in the group " << tmp_name
              << "; Fix the existing ones first.\n";
      has_warning_ = true;
      return;
    }

    auto name = name_node.as<std::string>();

    // Reserved group name validation
    if (name == "*") {
      errors_ << "E: Group name '*' is reserved; "
                 "Try to use some other else\n";
      has_error_ = true;
      return;
    }

    // Apply the group configuration to the logging system
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

    // Set the group as a fallback if specified
    if (is_fallback) {
      system_.setFallbackGroup(name);
    }

    // Recursively parse child groups
    if (children_node.IsDefined() and children_node.IsSequence()) {
      parseGroups(children_node, name);
    }
  }

}  // namespace soralog
