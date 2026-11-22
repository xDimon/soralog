/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/impl/sink_to_eventlog.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

// After soralog headers: windows.h defines macros named like Level values
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#undef IGNORE

using namespace soralog;

namespace {
  /// @brief Record of the Event Log, as much as the tests check
  struct Record {
    WORD type;
    DWORD event_id;
    std::wstring text;
  };

  /**
   * @brief Finds the latest record of the source in the Application log whose
   * first string contains the text
   */
  std::optional<Record> findRecord(const std::wstring &source,
                                   const std::wstring &text) {
    HANDLE log = OpenEventLogW(nullptr, L"Application");
    if (log == nullptr) {
      ADD_FAILURE() << "OpenEventLogW failed: " << GetLastError();
      return std::nullopt;
    }
    std::optional<Record> found;
    std::vector<BYTE> buff(1 << 16);
    size_t seen = 0;
    while (not found and seen < 100000) {
      DWORD read = 0;
      DWORD needed = 0;
      if (not ReadEventLogW(log,
                            EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                            0,
                            buff.data(),
                            static_cast<DWORD>(buff.size()),
                            &read,
                            &needed)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
          buff.resize(needed);
          continue;
        }
        break;  // ERROR_HANDLE_EOF: the whole log is read
      }
      for (DWORD offset = 0; offset < read and not found;) {
        const auto *record =
            reinterpret_cast<const EVENTLOGRECORD *>(buff.data() + offset);
        // The source name goes right after the fixed part of the record
        const auto *record_source =
            reinterpret_cast<const wchar_t *>(record + 1);
        if (record->NumStrings > 0 and source == record_source) {
          const auto *first_string = reinterpret_cast<const wchar_t *>(
              buff.data() + offset + record->StringOffset);
          std::wstring record_text(first_string);
          if (record_text.find(text) != std::wstring::npos) {
            found = Record{record->EventType,
                           record->EventID & 0xFFFF,
                           std::move(record_text)};
          }
        }
        offset += record->Length;
        ++seen;
      }
    }
    CloseEventLog(log);
    return found;
  }

  /// @brief Unique tag to find the records of this run
  std::string uniqueTag() {
    std::random_device random;
    return std::to_string(random()) + "_" + std::to_string(random());
  }

  /// @brief UTF-8 string from a u8 literal
  std::string utf8(std::u8string_view str) {
    return {str.begin(), str.end()};
  }

  std::wstring wide(const std::string &ascii) {
    return {ascii.begin(), ascii.end()};
  }
}  // namespace

/**
 * @test Logging
 * @brief Tests that a message reaches the Application log.
 *
 * @given An Event Log sink with an unregistered source and event ID 7.
 * @when An error with non-ASCII text is logged and the sink flushed.
 * @then The Application log has an error record of the source with event
 * ID 7 and the text.
 */
TEST(SinkToEventLogTest, Logging) {
  const auto tag = uniqueTag();
  const auto source = "soralog_test_" + tag;
  {
    auto sink = std::make_shared<SinkToEventLog>("eventlog",
                                                 Level::TRACE,
                                                 source,
                                                 Sink::ThreadInfoType::NONE,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 0,  // Write at once
                                                 std::nullopt,
                                                 7);
    sink->push("test", Level::ERROR, "message {} {}", tag, utf8(u8"éж"));
    sink->flush();
  }

  auto record = findRecord(wide(source), wide("message " + tag) + L" éж");
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(record->type, EVENTLOG_ERROR_TYPE);
  EXPECT_EQ(record->event_id, 7u);
}

/**
 * @test TraceIsNotLogged
 * @brief Tests that trace messages don't reach the Event Log.
 *
 * @given An Event Log sink.
 * @when A trace message and then a warning are logged.
 * @then Only the warning is in the Application log, as a warning record.
 */
TEST(SinkToEventLogTest, TraceIsNotLogged) {
  const auto tag = uniqueTag();
  const auto source = "soralog_test_" + tag;
  {
    auto sink = std::make_shared<SinkToEventLog>("eventlog",
                                                 Level::TRACE,
                                                 source,
                                                 Sink::ThreadInfoType::NONE,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 std::nullopt,
                                                 0);
    sink->push("test", Level::TRACE, "trace {}", tag);
    sink->push("test", Level::WARN, "warning {}", tag);
    sink->flush();
  }

  EXPECT_FALSE(findRecord(wide(source), wide("trace " + tag)).has_value());
  auto record = findRecord(wide(source), wide("warning " + tag));
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(record->type, EVENTLOG_WARNING_TYPE);
  EXPECT_EQ(record->event_id, 0u);
}

/**
 * @test YamlConfig
 * @brief Tests the Event Log sink configured by YAML.
 *
 * @given A YAML config with an 'eventlog' sink, its source and event ID.
 * @when An info message is logged.
 * @then The Application log has an information record of the source with
 * the event ID and the text.
 */
TEST(SinkToEventLogTest, YamlConfig) {
  const auto tag = uniqueTag();
  const auto source = "soralog_test_" + tag;
  {
    LoggingSystem system(
        std::make_shared<ConfiguratorFromYAML>(
            std::string("sinks:\n"
                        "  - name: eventlog\n"
                        "    type: eventlog\n"
                        "    source: ")
            + source
            + "\n"
              "    event_id: 42\n"
              "    latency: 0\n"
              "groups:\n"
              "  - name: main\n"
              "    sink: eventlog\n"
              "    level: trace\n"
              "    is_fallback: true\n"));
    auto result = system.configure();
    EXPECT_FALSE(result.has_error) << result.message;
    EXPECT_FALSE(result.has_warning) << result.message;
    auto logger = system.getLogger("test", "main");
    logger->info("info {}", tag);
    logger->flush();
  }

  auto record = findRecord(wide(source), wide("info " + tag));
  ASSERT_TRUE(record.has_value());
  EXPECT_EQ(record->type, EVENTLOG_INFORMATION_TYPE);
  EXPECT_EQ(record->event_id, 42u);
}
