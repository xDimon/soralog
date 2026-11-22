/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <system_error>

#include <soralog/impl/configurator_from_yaml.hpp>
#include <soralog/logger.hpp>
#include <soralog/logging_system.hpp>

using namespace soralog;

namespace {
  /// @brief Path as UTF-8, the encoding of YAML text
  std::string toUtf8(const std::filesystem::path &path) {
    auto str = path.u8string();
    return {str.begin(), str.end()};
  }
}  // namespace

/**
 * @test NonAsciiPaths
 * @brief Tests a config file and a log file with non-ASCII paths.
 *
 * Narrow strings are UTF-8 on POSIX, so the test matters on Windows, where
 * they are in the ANSI code page.
 *
 * @given A config file in a directory named with a Latin and a Cyrillic letter
 * beyond ASCII, which no single ANSI code page holds both of, defining a file
 * sink in the same directory.
 * @when The logging system is configured from the file and a message logged.
 * @then The configuration succeeds and the message is in the log file.
 */
TEST(ConfiguratorFromYamlTest, NonAsciiPaths) {
  std::random_device random;
  auto dir = std::filesystem::temp_directory_path()
           / std::filesystem::path(u8"soralog_test_éж_");
  dir += std::to_string(random());
  ASSERT_TRUE(std::filesystem::create_directory(dir));
  const auto config_path = dir / "config.yml";
  const auto log_path = dir / "log.txt";

  {
    std::ofstream config(config_path);
    // Single-quoted: no escapes there, Windows paths have backslashes
    config << "sinks:\n"
              "  - name: file\n"
              "    type: file\n"
              "    path: '"
           << toUtf8(log_path)
           << "'\n"
              "    latency: 0\n"
              "    level: trace\n"
              "groups:\n"
              "  - name: main\n"
              "    sink: file\n"
              "    level: trace\n"
              "    is_fallback: true\n";
    ASSERT_TRUE(config.good());
  }

  {
    LoggingSystem system(std::make_shared<ConfiguratorFromYAML>(config_path));
    auto result = system.configure();
    EXPECT_FALSE(result.has_error) << result.message;

    auto logger = system.getLogger("test", "main");
    logger->info("Message to a non-ASCII path");
    logger->flush();
  }

  std::ifstream log(log_path);
  const std::string content{std::istreambuf_iterator<char>(log),
                            std::istreambuf_iterator<char>()};
  EXPECT_NE(content.find("Message to a non-ASCII path"), std::string::npos);
  log.close();

  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
}
