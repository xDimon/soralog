/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <system_error>

namespace soralog {

  /**
   * @class FileWriter
   * @brief Appends to a file, without buffering of its own.
   *
   * Every write() goes to the system at once. On Windows the file is open
   * with FILE_SHARE_DELETE, so that it can be renamed or deleted while open,
   * as on POSIX: external rotation works there too.
   */
  class FileWriter final {
   public:
    /**
     * @struct WriteResult
     * @brief Outcome of write().
     */
    struct WriteResult {
      /// Bytes written, before the error if there is one
      size_t written = 0;
      /// Error, if any; disk full is std::errc::no_space_on_device everywhere
      std::error_code error;
    };

    FileWriter() noexcept = default;
    FileWriter(FileWriter &&other) noexcept;
    FileWriter(const FileWriter &) = delete;
    FileWriter &operator=(FileWriter &&other) noexcept;
    FileWriter &operator=(const FileWriter &) = delete;

    /**
     * @brief Closes the file.
     */
    ~FileWriter();

    /**
     * @brief Opens the file for appending, creating it if needed.
     *
     * The file open before is closed only if the new one is open, otherwise
     * it stays.
     * @param path Path to the file.
     * @return Error, if the file can't be open.
     */
    std::error_code open(const std::filesystem::path &path) noexcept;

    /**
     * @return True if a file is open.
     */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * @brief Closes the file, if open.
     */
    void close() noexcept;

    /**
     * @brief Appends data to the file.
     *
     * Repeats after partial writes and interruptions until all is written or
     * an error happens.
     * @param data Data to write.
     * @return Number of bytes written and the error, if any.
     */
    WriteResult write(std::span<const char> data) noexcept;

   private:
#ifdef _WIN32
    /// HANDLE of the file, nullptr if closed; void * to keep windows.h out
    void *handle_ = nullptr;
#else
    /// Descriptor of the file, -1 if closed
    int fd_ = -1;
#endif
  };

}  // namespace soralog
