/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/impl/file_writer.hpp>

#include <algorithm>
#include <limits>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#endif

namespace soralog {

  namespace {

#ifdef _WIN32
    /// Error of the last failed system call
    std::error_code lastError() noexcept {
      const auto code = GetLastError();
      if (code == ERROR_DISK_FULL or code == ERROR_HANDLE_DISK_FULL) {
        // Explicitly: mapping of system errors to std::errc differs
        // between the standard libraries
        return std::make_error_code(std::errc::no_space_on_device);
      }
      return {static_cast<int>(code), std::system_category()};
    }
#else
    /// Error of the last failed system call
    std::error_code lastError() noexcept {
      return {errno, std::generic_category()};
    }
#endif

  }  // namespace

  FileWriter::FileWriter(FileWriter &&other) noexcept
#ifdef _WIN32
      : handle_(std::exchange(other.handle_, nullptr))
#else
      : fd_(std::exchange(other.fd_, -1))
#endif
  {
  }

  FileWriter &FileWriter::operator=(FileWriter &&other) noexcept {
    // The file open before is closed by the temporary
    FileWriter tmp(std::move(other));
#ifdef _WIN32
    std::swap(handle_, tmp.handle_);
#else
    std::swap(fd_, tmp.fd_);
#endif
    return *this;
  }

  FileWriter::~FileWriter() {
    close();
  }

  std::error_code FileWriter::open(const std::filesystem::path &path) noexcept {
    FileWriter file;
#ifdef _WIN32
    // FILE_APPEND_DATA without FILE_WRITE_DATA: every write goes to the end
    HANDLE handle =
        CreateFileW(path.c_str(),
                    FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
      return lastError();
    }
    file.handle_ = handle;
#else
    constexpr int flags = O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC;
    // As for std::ofstream, the umask cuts it down
    constexpr mode_t mode = 0666;
    int descriptor = -1;
    while (true) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
      descriptor = ::open(path.c_str(), flags, mode);
      if (descriptor != -1 or errno != EINTR) {
        break;
      }
    }
    if (descriptor == -1) {
      return lastError();
    }
    file.fd_ = descriptor;
#endif
    *this = std::move(file);
    return {};
  }

  bool FileWriter::is_open() const noexcept {
#ifdef _WIN32
    return handle_ != nullptr;
#else
    return fd_ != -1;
#endif
  }

  void FileWriter::close() noexcept {
    if (not is_open()) {
      return;
    }
#ifdef _WIN32
    CloseHandle(handle_);
    handle_ = nullptr;
#else
    ::close(fd_);
    fd_ = -1;
#endif
  }

  FileWriter::WriteResult FileWriter::write(
      std::span<const char> data) noexcept {
    WriteResult result;
    if (not is_open()) {
      result.error = std::make_error_code(std::errc::bad_file_descriptor);
      return result;
    }
    while (result.written < data.size()) {
      const auto rest = data.subspan(result.written);
#ifdef _WIN32
      DWORD written = 0;
      const auto size = static_cast<DWORD>(
          std::min<size_t>(rest.size(), std::numeric_limits<DWORD>::max()));
      if (not WriteFile(handle_, rest.data(), size, &written, nullptr)) {
        result.error = lastError();
        break;
      }
      if (written == 0) {  // Never expected for a file; don't spin on it
        result.error = std::make_error_code(std::errc::io_error);
        break;
      }
#else
      const auto written = ::write(fd_, rest.data(), rest.size());
      if (written < 0) {
        if (errno == EINTR) {
          continue;
        }
        result.error = lastError();
        break;
      }
      if (written == 0) {  // Never expected for a file; don't spin on it
        result.error = std::make_error_code(std::errc::io_error);
        break;
      }
#endif
      result.written += static_cast<size_t>(written);
    }
    return result;
  }

}  // namespace soralog
