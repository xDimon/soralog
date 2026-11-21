/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace soralog::util {

  /**
   * @brief Retrieves a unique thread number.
   *
   * Each thread receives a unique, incrementing identifier upon first call.
   * Defined out of line: the counter and the number of a thread must be one
   * for the whole process, also when soralog is a set of DLLs.
   * @return Unique thread number.
   */
  size_t getThreadNumber();

  /**
   * @brief Sets the name of the current thread.
   *
   * The thread name is truncated if it exceeds 15 characters.
   * @param name Desired thread name.
   */
  void setThreadName(std::string_view name);

  /**
   * @brief Retrieves the name of the current thread.
   *
   * If the platform does not support thread names, a generated name
   * (e.g., "Thread#1") is used.
   * @param name Output parameter for the thread name.
   */
  void getThreadName(std::array<char, 16> &name);

  /**
   * @brief Retrieves the name of the current thread as a string.
   *
   * @return The thread name as the system reports it; a generated name on
   * platforms without thread names.
   */
  std::string getThreadName();

}  // namespace soralog::util
