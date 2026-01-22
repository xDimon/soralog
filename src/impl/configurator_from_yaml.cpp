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

#include <fmt/format.h>

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
#define WITHOUT_TRACE_LOG_LEVEL
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

  void ConfiguratorFromYAML::prepare(LoggingSystem &system,
                                     size_t index,
                                     Result &result) {
    YAML::Node node;
    std::visit(
        [&](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::filesystem::path>) {
            // Load YAML from a file
            try {
              node = YAML::LoadFile(arg);
            } catch (const std::exception &exception) {
              result.message +=
                  fmt::format("E:{}: Can't parse file '{}': {}\n",
                              index + 1,
                              std::filesystem::weakly_canonical(arg).string(),
                              exception.what());
              result.has_error = true;
            }
          } else if constexpr (std::is_same_v<T, std::string>) {
            // Load YAML from a string
            try {
              node = YAML::Load(arg);
            } catch (const std::exception &exception) {
              result.message += fmt::format("E:{}: Can't parse content: {}\n",
                                            index + 1,
                                            exception.what());
              result.has_error = true;
            }
          } else if constexpr (std::is_same_v<T, YAML::Node>) {
            // Use the provided YAML node directly
            node = arg;
          } else {
            static_assert(always_false_v<T>, "Unhandled configuration type!");
          }
        },
        config_);

    if (not node.IsMap()) {
      result.message +=
          fmt::format("E:{}: Config is not a YAML map\n", index + 1);
      result.has_error = true;
      return;
    }

    // Validate top-level keys
    for (const auto &it : node) {
      auto key = it.first.as<std::string>();
      if (key == "sinks" or key == "groups") {
        continue;
      }
      result.message +=
          fmt::format("W:{}: Unknown property: {}\n", index + 1, key);
      result.has_warning = true;
    }

    applicator_ =
        std::make_shared<Applicator>(std::move(node), system, index, result);
  }

  void ConfiguratorFromYAML::applySinks() const {
    if (applicator_) {
      applicator_->parseSinks();
    }
  }

  void ConfiguratorFromYAML::applyGroups() const {
    if (applicator_) {
      applicator_->parseGroups();
    }
  }

  void ConfiguratorFromYAML::cleanup() {
    applicator_.reset();
  }

  void ConfiguratorFromYAML::Applicator::parseSinks() {
    auto sinks = node["sinks"];
    if (sinks.IsDefined()) {
      parseSinks(sinks);
    }
  }

  void ConfiguratorFromYAML::Applicator::parseGroups() {
    auto groups = node["groups"];
    if (groups.IsDefined()) {
      parseGroups(groups, {});
    }
  }

  void ConfiguratorFromYAML::Applicator::parseSinks(const YAML::Node &sinks) {
    if (sinks.IsNull()) {
      res.message += fmt::format("E:{}: Sinks list is empty\n", id);
      res.has_error = true;
      return;
    }

    if (not sinks.IsSequence()) {
      res.message += fmt::format("E:{}: Sinks is not a YAML sequence\n", id);
      res.has_error = true;
      return;
    }

    for (size_t i = 0; i < sinks.size(); ++i) {
      auto sink = sinks[i];
      if (not sink.IsMap()) {
        res.message += fmt::format(
            "W:{}: Element #{} of 'sinks' is not a YAML map\n", id, i);
        res.has_warning = true;
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
      res.message += fmt::format(
          "E:{}: Property 'level' of {} is not scalar\n", id, target);
      res.has_error = true;
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
        res.message += fmt::format(
            "W:{}: Level 'debug' in {} won't work: "
            "it has been disabled with a compile-time option\n",
            id,
            target);
        res.has_warning = true;
      }
      return Level::DEBUG;
    }
    if (level_string == "trace") {
      if constexpr (trace_level_disabled) {
        res.message += fmt::format(
            "W:{}: Level 'trace' in {} won't work: "
            "it has been disabled with a compile-time option\n",
            id,
            target);
        res.has_warning = true;
      }
      return Level::TRACE;
    }
    res.message += fmt::format(
        "E:{}: Invalid level in {}: {}\n", id, target, level_string);
    res.has_error = true;
    return std::nullopt;
  }

  void ConfiguratorFromYAML::Applicator::parseSink(size_t number,
                                                   const YAML::Node &sink) {
    bool fail = false;

    // Extract and validate the sink name
    auto name_node = sink["name"];
    if (not name_node.IsDefined()) {
      res.message +=
          fmt::format("E:{}: Not found 'name' of sink node #{}\n", id, number);
      fail = true;
    } else if (not name_node.IsScalar()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'name' of sink node #{} is not scalar\n", id, number);
      res.has_error = true;
    }

    // Extract and validate the sink type
    auto type_node = sink["type"];
    if (not type_node.IsDefined()) {
      fail = true;
      res.message +=
          fmt::format("E:{}: Not found 'type' of sink node #{}\n", id, number);
      res.has_error = true;
    } else if (not type_node.IsScalar()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'type' of sink node #{} is not scalar\n", id, number);
      res.has_error = true;
    }

    if (fail) {
      return;
    }

    auto name = name_node.as<std::string>();
    auto type = type_node.as<std::string>();

    // Ensure the sink name is not reserved
    if (name == "*") {
      res.message += fmt::format(
          "E:{}: Sink name '*' is reserved; Try to use some other name\n", id);
      res.has_error = true;
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
      res.message += fmt::format(
          "E:{}: Unknown 'type' of sink node '{}': {}\n", id, name, type);
      res.has_error = true;
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
        res.message += fmt::format(
            "W:{}: Property 'color' of sink node is not true or false\n", id);
        res.has_warning = true;
      } else {
        color = color_node.as<bool>();
      }
    }

    // Parse 'stream' option (stdout/stderr)
    auto stream_node = sink_node["stream"];
    if (stream_node.IsDefined()) {
      if (not stream_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'stream' of sink node is not stdout or stderr\n",
            id);
        res.has_warning = true;
      } else {
        auto stream_str = stream_node.as<std::string>();
        if (stream_str == "stdout") {
          stream_type = SinkToConsole::Stream::STDOUT;
        } else if (stream_str == "stderr") {
          stream_type = SinkToConsole::Stream::STDERR;
        } else {
          res.message += fmt::format(
              "W:{}: Invalid 'stream' value: expected 'stdout' or 'stderr'\n",
              id);
          res.has_warning = true;
        }
      }
    }

    // Parse 'thread' information type
    auto thread_node = sink_node["thread"];
    if (thread_node.IsDefined()) {
      if (not thread_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'thread' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto thread_str = thread_node.as<std::string>();
        if (thread_str == "name") {
          thread_info_type = Sink::ThreadInfoType::NAME;
        } else if (thread_str == "id") {
          thread_info_type = Sink::ThreadInfoType::ID;
        } else if (thread_str != "none") {
          res.message +=
              fmt::format("W:{}: Invalid 'thread' value of sink '{}': {}\n",
                          id,
                          name,
                          thread_str);
          res.has_warning = true;
        }
      }
    }

    // Parse capacity settings
    auto capacity_node = sink_node["capacity"];
    if (capacity_node.IsDefined()) {
      if (not capacity_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'capacity' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto capacity_int = capacity_node.as<int>();
        if (capacity_int >= 4) {
          capacity.emplace(capacity_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'capacity' value of sink '{}': {}\n",
                          id,
                          name,
                          capacity_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse buffer size settings
    auto buffer_node = sink_node["buffer"];
    if (buffer_node.IsDefined()) {
      if (not buffer_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'buffer' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto buffer_int = buffer_node.as<int>();
        if (buffer_int >= sizeof(Event) * 4) {
          buffer_size.emplace(buffer_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'buffer' value of sink '{}': {}\n",
                          id,
                          name,
                          buffer_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse max message length settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'max_message_length' of sink node is not scalar\n",
            id);
        res.has_warning = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int > 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          res.message += fmt::format(
              "W:{}: Invalid 'max_message_length' value of sink '{}': {}\n",
              id,
              name,
              max_message_length_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse latency settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'latency' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          res.message +=
              fmt::format("W:{}: Invalid 'latency' value of sink '{}': {}\n",
                          id,
                          name,
                          latency_node.as<std::string>());
          res.has_warning = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse fault reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'at_fault' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          res.message +=
              fmt::format("W:{}: Invalid 'at_fault' value of sink '{}': {}\n",
                          id,
                          name,
                          at_fault_str);
          res.has_warning = true;
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
        res.message += fmt::format(
            "W:{}: Unknown property of sink '{}': {}\n", id, name, key);
        res.has_warning = true;
      }
    }

    // Check if the sink already exists
    if (sys.getSink(name)) {
      res.message += fmt::format(
          "W:{}: Sink with name '{}' already exists; "
          "overriding previous version\n",
          id,
          name);
      res.has_warning = true;
    }

    // Create the console sink
    sys.makeSink<SinkToConsole>(name,
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
      res.message +=
          fmt::format("E:{}: Not found 'path' of sink '{}'\n", id, name);
      res.has_error = true;
    } else if (not path_node.IsScalar()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'path' of sink '{}' is not scalar\n", id, name);
      res.has_error = true;
    }

    // Parse 'thread' information type
    auto thread_node = sink_node["thread"];
    if (thread_node.IsDefined()) {
      if (not thread_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'thread' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto thread_str = thread_node.as<std::string>();
        if (thread_str == "name") {
          thread_info_type = Sink::ThreadInfoType::NAME;
        } else if (thread_str == "id") {
          thread_info_type = Sink::ThreadInfoType::ID;
        } else if (thread_str != "none") {
          res.message +=
              fmt::format("W:{}: Invalid 'thread' value of sink '{}': {}\n",
                          id,
                          name,
                          thread_str);
          res.has_warning = true;
        }
      }
    }

    // Parse 'capacity' settings
    auto capacity_node = sink_node["capacity"];
    if (capacity_node.IsDefined()) {
      if (not capacity_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'capacity' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto capacity_int = capacity_node.as<int>();
        if (capacity_int >= 4) {
          capacity.emplace(capacity_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'capacity' value of sink '{}': {}\n",
                          id,
                          name,
                          capacity_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'buffer' size settings
    auto buffer_node = sink_node["buffer"];
    if (buffer_node.IsDefined()) {
      if (not buffer_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'buffer' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto buffer_int = buffer_node.as<int>();
        if (buffer_int >= sizeof(Event) * 4) {
          buffer_size.emplace(buffer_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'buffer' value of sink '{}': {}\n",
                          id,
                          name,
                          buffer_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'max_message_length' settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'max_message_length' of sink node is not scalar\n",
            id);
        res.has_warning = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int >= 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          res.message += fmt::format(
              "W:{}: Invalid 'max_message_length' value of sink '{}': {}\n",
              id,
              name,
              max_message_length_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'latency' settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'latency' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          res.message +=
              fmt::format("W:{}: Invalid 'latency' value of sink '{}': {}\n",
                          id,
                          name,
                          latency_node.as<std::string>());
          res.has_warning = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse 'at_fault' reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'at_fault' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          res.message +=
              fmt::format("W:{}: Invalid 'at_fault' value of sink '{}': {}\n",
                          id,
                          name,
                          at_fault_str);
          res.has_warning = true;
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
        res.message += fmt::format(
            "W:{}: Unknown property of sink '{}': {}\n", id, name, key);
        res.has_warning = true;
      }
    }

    if (fail) {
      return;
    }

    auto path = path_node.as<std::string>();

    // Check if the sink already exists
    if (sys.getSink(name)) {
      res.message += fmt::format(
          "W:{}: Sink with name '{}' already exists; "
          "overriding previous version\n",
          id,
          name);
      res.has_warning = true;
    }

    // Create the file sink
    sys.makeSink<SinkToFile>(name,
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
      res.message +=
          fmt::format("E:{}: Not found 'ident' of sink '{}'\n", id, name);
      res.has_error = true;
    } else if (not ident_node.IsScalar()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'ident' of sink '{}' is not scalar\n", id, name);
      res.has_error = true;
    }

    // Parse 'thread' information type
    auto thread_node = sink_node["thread"];
    if (thread_node.IsDefined()) {
      if (not thread_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'thread' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto thread_str = thread_node.as<std::string>();
        if (thread_str == "name") {
          thread_info_type = Sink::ThreadInfoType::NAME;
        } else if (thread_str == "id") {
          thread_info_type = Sink::ThreadInfoType::ID;
        } else if (thread_str != "none") {
          res.message +=
              fmt::format("W:{}: Invalid 'thread' value of sink '{}': {}\n",
                          id,
                          name,
                          thread_str);
          res.has_warning = true;
        }
      }
    }

    // Parse 'capacity' settings
    auto capacity_node = sink_node["capacity"];
    if (capacity_node.IsDefined()) {
      if (not capacity_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'capacity' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto capacity_int = capacity_node.as<int>();
        if (capacity_int >= 4) {
          capacity.emplace(capacity_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'capacity' value of sink '{}': {}\n",
                          id,
                          name,
                          capacity_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'buffer' size settings
    auto buffer_node = sink_node["buffer"];
    if (buffer_node.IsDefined()) {
      if (not buffer_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'buffer' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto buffer_int = buffer_node.as<int>();
        if (buffer_int >= sizeof(Event) * 4) {
          buffer_size.emplace(buffer_int);
        } else {
          res.message +=
              fmt::format("W:{}: Invalid 'buffer' value of sink '{}': {}\n",
                          id,
                          name,
                          buffer_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'max_message_length' settings
    auto max_message_length_node = sink_node["max_message_length"];
    if (max_message_length_node.IsDefined()) {
      if (not max_message_length_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'max_message_length' of sink node is not scalar\n",
            id);
        res.has_warning = true;
      } else {
        auto max_message_length_int = max_message_length_node.as<int>();
        if (max_message_length_int >= 64) {
          max_message_length.emplace(max_message_length_int);
        } else {
          res.message += fmt::format(
              "W:{}: Invalid 'max_message_length' value of sink '{}': {}\n",
              id,
              name,
              max_message_length_node.as<std::string>());
          res.has_warning = true;
        }
      }
    }

    // Parse 'latency' settings
    auto latency_node = sink_node["latency"];
    if (latency_node.IsDefined()) {
      if (not latency_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'latency' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto latency_int = latency_node.as<int>();
        if (std::to_string(latency_int) != latency_node.as<std::string>()
            or latency_int < 0) {
          res.message +=
              fmt::format("W:{}: Invalid 'latency' value of sink '{}': {}\n",
                          id,
                          name,
                          latency_node.as<std::string>());
          res.has_warning = true;
        } else {
          latency.emplace(latency_int);
        }
      }
    }

    // Parse 'at_fault' reaction type
    auto at_fault_node = sink_node["at_fault"];
    if (at_fault_node.IsDefined()) {
      if (not at_fault_node.IsScalar()) {
        res.message += fmt::format(
            "W:{}: Property 'at_fault' of sink node is not scalar\n", id);
        res.has_warning = true;
      } else {
        auto at_fault_str = at_fault_node.as<std::string>();
        if (at_fault_str == "terminate") {
          at_fault = Sink::AtFaultReactionType::TERMINATE;
        } else if (at_fault_str == "ignore") {
          at_fault = Sink::AtFaultReactionType::IGNORE;
        } else if (at_fault_str != "wait") {
          res.message +=
              fmt::format("W:{}: Invalid 'at_fault' value of sink '{}': {}\n",
                          id,
                          name,
                          at_fault_str);
          res.has_warning = true;
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
        res.message += fmt::format(
            "W:{}: Unknown property of sink '{}': {}\n", id, name, key);
        res.has_warning = true;
      }
    }

    if (fail) {
      return;
    }

    auto ident = ident_node.as<std::string>();

    // Check if the sink already exists
    if (sys.getSink(name)) {
      res.message += fmt::format(
          "W:{}: Sink with name '{}' already exists; "
          "overriding previous version\n",
          id,
          name);
      res.has_warning = true;
    }

    // Create the syslog sink
    sys.makeSink<SinkToSyslog>(name,
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
      res.message +=
          fmt::format("E:{}: Not found 'sinks' of sink '{}'\n", id, name);
      res.has_error = true;
    } else if (not sinks_node.IsSequence()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'sinks' of sink '{}' is not a list\n", id, name);
      res.has_error = true;
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
        res.message += fmt::format(
            "W:{}: Unknown property of sink '{}': {}\n", id, name, key);
        res.has_warning = true;
      }
    }

    if (fail) {
      return;
    }

    // Extract sink names and resolve them to actual sink objects
    auto sink_names = sinks_node.as<std::vector<std::string>>();

    std::vector<std::shared_ptr<Sink>> sinks;
    for (auto &sink_name : sink_names) {
      auto sink = sys.getSink(sink_name);
      if (not sink) {
        res.message +=
            fmt::format("E:{}: Sink '{}' must be defined before sink '{}'\n",
                        id,
                        sink_name,
                        name);
        res.has_error = true;
      } else {
        sinks.emplace_back(std::move(sink));
      }
    }

    // Create the multisink
    sys.makeSink<Multisink>(name, level, std::move(sinks));
  }

  void ConfiguratorFromYAML::Applicator::parseGroups(
      const YAML::Node &groups, const std::optional<std::string> &parent) {
    if (groups.IsNull()) {
      res.message += fmt::format("E:{}: Node 'groups' is empty\n", id);
      res.has_error = true;
      return;
    }

    if (not groups.IsSequence()) {
      res.message += fmt::format("E:{}: Node 'groups' is not a sequence\n", id);
      res.has_error = true;
      return;
    }

    // Iterate over the group list and parse each entry
    for (size_t i = 0; i < groups.size(); ++i) {
      auto group = groups[i];
      if (not group.IsMap()) {
        res.message +=
            fmt::format("E:{}: Element #{} of 'groups' is not a map\n", id, i);
        res.has_error = true;
        continue;
      }
      parseGroup(i, group, parent);
    }
  }

  void ConfiguratorFromYAML::Applicator::parseGroup(
      size_t number,
      const YAML::Node &group_node,
      const std::optional<std::string> &parent) {
    bool fail = false;
    bool is_fallback = false;

    // Extract and validate the group name
    auto name_node = group_node["name"];
    std::string tmp_name = "node #" + std::to_string(number);
    if (not name_node.IsDefined()) {
      fail = true;
      res.message +=
          fmt::format("E:{}: Not found 'name' of group {}\n", id, tmp_name);
      res.has_error = true;
    } else if (not name_node.IsScalar()) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Property 'name' of group {} is not scalar\n", id, tmp_name);
      res.has_error = true;
    } else {
      tmp_name = "'" + name_node.as<std::string>() + "'";
    }

    // Check if the group is marked as a fallback group
    auto fallback_node = group_node["is_fallback"];
    if (fallback_node.IsDefined()) {
      if (not fallback_node.IsScalar()) {
        fail = true;
        res.message += fmt::format(
            "E:{}: Property 'is_fallback' of group {} is not scalar\n",
            id,
            tmp_name);
        res.has_error = true;
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
        res.message += fmt::format(
            "E:{}: Property 'sink' of group {} is not scalar\n", id, tmp_name);
        res.has_error = true;
      } else {
        sink.emplace(sink_node.as<std::string>());
        // Validate the sink existence
        if (not sys.getSink(sink.value())) {
          fail = true;
          res.message +=
              fmt::format("E:{}: Sink '{}' of group {} is undefined\n",
                          id,
                          *sink,
                          tmp_name);
          res.has_error = true;
        }
      }
    } else if (not parent) {
      sink.emplace("*");
    }

    // Extract and validate the level property
    auto level_node = group_node["level"];
    if (not level_node.IsDefined() and not parent) {
      fail = true;
      res.message += fmt::format(
          "E:{}: Not found 'level' of root group {}\n", id, tmp_name);
      res.has_error = true;
    }
    auto level = parseLevel(fmt::format("group {}", tmp_name), group_node);

    // Validate the children property
    auto children_node = group_node["children"];
    if (children_node.IsDefined()) {
      if (not children_node.IsNull() and not children_node.IsSequence()) {
        fail = true;
        res.message += fmt::format(
            "E:{}: Property 'children' of group {} is not sequence\n",
            id,
            tmp_name);
        res.has_error = true;
      }
    }

    // Check for unknown properties
    static constexpr std::array known_properties = {
        "name", "is_fallback", "sink", "level", "children"};
    for (const auto &it : group_node) {
      auto key = it.first.as<std::string>();
      if (std::ranges::find(known_properties, key) == known_properties.end()) {
        res.message += fmt::format(
            "W:{}: Unknown property of group {}: {}\n", id, tmp_name, key);
        res.has_warning = true;
      }
    }

    if (fail) {
      res.message += fmt::format(
          "W:{}: There are probably more bugs in the group {}; "
          "Fix the existing ones first.\n",
          id,
          tmp_name);
      res.has_warning = true;
      return;
    }

    auto name = name_node.as<std::string>();

    // Reserved group name validation
    if (name == "*") {
      res.message += fmt::format(
          "E:{}: Group name '*' is reserved; "
          "Try to use some other else\n",
          id);
      res.has_error = true;
      return;
    }

    // Apply the group configuration to the logging system
    if (sys.getGroup(name)) {
      if (parent.has_value()) {
        sys.setParentOfGroup(name, parent.value());
      }
      if (sink.has_value() and sink.value() != "*") {
        sys.setSinkOfGroup(name, sink.value());
      }
      if (level.has_value()) {
        sys.setLevelOfGroup(name, level.value());
      }
    } else {
      sys.makeGroup(name, parent, sink, level);
    }

    // Set the group as a fallback if specified
    if (is_fallback) {
      sys.setFallbackGroup(name);
    }

    // Recursively parse child groups
    if (children_node.IsDefined() and children_node.IsSequence()) {
      parseGroups(children_node, name);
    }
  }

}  // namespace soralog
