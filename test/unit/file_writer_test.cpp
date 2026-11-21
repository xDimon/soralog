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
#include <string_view>
#include <system_error>

#include "soralog/impl/file_writer.hpp"

using namespace soralog;

/**
 * @class FileWriterTest
 * @brief Test fixture giving a unique path for a temporary file.
 */
class FileWriterTest : public ::testing::Test {
 public:
  void SetUp() override {
    std::random_device random;
    path_ = std::filesystem::temp_directory_path()
          / ("soralog_test_" + std::to_string(random()) + "_"
             + std::to_string(random()));
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }

  /// @brief Reads the whole file
  static std::string read(const std::filesystem::path &path) {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
  }

 protected:
  std::filesystem::path path_;
};

/**
 * @test Appending
 * @brief Tests that the file is created and then appended to.
 *
 * @given A file written by one writer and closed.
 * @when Another writer opens it and writes.
 * @then The file holds both writes, the first one first.
 */
TEST_F(FileWriterTest, Appending) {
  {
    FileWriter writer;
    ASSERT_FALSE(writer.open(path_));
    auto result = writer.write(std::string_view("first\n"));
    EXPECT_FALSE(result.error);
    EXPECT_EQ(result.written, 6u);
  }
  {
    FileWriter writer;
    ASSERT_FALSE(writer.open(path_));
    EXPECT_FALSE(writer.write(std::string_view("second\n")).error);
  }
  EXPECT_EQ(read(path_), "first\nsecond\n");
}

/**
 * @test FailedOpenKeepsFile
 * @brief Tests that a failed open() leaves the file open before.
 *
 * @given A writer with a file open.
 * @when It fails to open a file in a directory which does not exist.
 * @then An error is returned and writes still go to the first file.
 */
TEST_F(FileWriterTest, FailedOpenKeepsFile) {
  FileWriter writer;
  ASSERT_FALSE(writer.open(path_));
  EXPECT_TRUE(writer.open(std::filesystem::path(path_.string() + "_no_such_dir")
                          / "file"));
  EXPECT_TRUE(writer.is_open());
  EXPECT_FALSE(writer.write(std::string_view("still here\n")).error);
  writer.close();
  EXPECT_EQ(read(path_), "still here\n");
}

/**
 * @test WriteWhenClosed
 * @brief Tests that writing without a file open is an error, not a crash.
 */
TEST_F(FileWriterTest, WriteWhenClosed) {
  FileWriter writer;
  auto result = writer.write(std::string_view("lost\n"));
  EXPECT_EQ(result.error, std::errc::bad_file_descriptor);
  EXPECT_EQ(result.written, 0u);
}

/**
 * @test RemovingOpenFile
 * @brief Tests that the open file can be removed, as on POSIX.
 *
 * On Windows it needs FILE_SHARE_DELETE; external rotation relies on it.
 *
 * @given A writer with a file open.
 * @when The file is removed.
 * @then The removal succeeds and the writer can still write.
 */
TEST_F(FileWriterTest, RemovingOpenFile) {
  FileWriter writer;
  ASSERT_FALSE(writer.open(path_));
  std::error_code ec;
  EXPECT_TRUE(std::filesystem::remove(path_, ec)) << ec.message();
  EXPECT_FALSE(writer.write(std::string_view("to nowhere\n")).error);
}

#ifdef __linux__
/**
 * @test DiskFull
 * @brief Tests that the disk full is reported as such: SinkToFile waits for
 * free space then instead of reopening the file.
 *
 * @given A writer to /dev/full, which fails every write with ENOSPC.
 * @when Data is written.
 * @then The error is std::errc::no_space_on_device.
 */
TEST(FileWriterDeviceTest, DiskFull) {
  FileWriter writer;
  ASSERT_FALSE(writer.open("/dev/full"));
  auto result = writer.write(std::string_view("data\n"));
  EXPECT_EQ(result.error, std::errc::no_space_on_device);
  EXPECT_EQ(result.written, 0u);
}
#endif
