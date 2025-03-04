/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <pthread.h>
#include <array>
#include <atomic>
#include <cstring>
#include <string>

namespace soralog::util {

  /**
   * @brief Retrieves a unique thread number.
   *
   * Each thread receives a unique, incrementing identifier upon first call.
   * @return Unique thread number.
   */
  inline size_t getThreadNumber() {
    static std::atomic_size_t tid_counter = 0;
    static thread_local size_t tid = ++tid_counter;
    return tid;
  }

  /**
   * @brief Sets the name of the current thread.
   *
   * The thread name is truncated if it exceeds 15 characters.
   * @param name Desired thread name.
   */
  inline void setThreadName(std::string_view name) {
    std::array<char, 16> buff{};
    memcpy(buff.data(), name.data(), std::min<size_t>(name.size(), 15));

#if defined(__linux__)
    pthread_setname_np(pthread_self(), buff.data());
#elif defined(__APPLE__)
    pthread_setname_np(buff.data());
#else
#warning \
    "Function setThreadName() is not implemented for this platform; \
    An auto-generated name will be used instead"
#endif
  }

  /**
   * @brief Retrieves the name of the current thread.
   *
   * If the platform does not support thread names, a generated name
   * (e.g., "Thread#1") is used.
   * @param name Output parameter for the thread name.
   */
  inline void getThreadName(std::array<char, 16> &name) {
    static thread_local std::array<char, 16> thr_name{};
    static thread_local bool initialized = [&] {
#if defined(__linux__) or defined(__APPLE__)
      pthread_getname_np(pthread_self(), thr_name.data(), thr_name.size());
#else
#warning \
    "Function getThreadName() is not implemented for this platform; An auto-generated name will be used instead"
      auto generated = "Thread#" + std::to_string(getThreadNumber());
      memcpy(thr_name.data(),
             generated.data(),
             std::min(generated.size(), thr_name.size()));
#endif
      return true;
    }();
    name = thr_name;
  }

  /**
   * @brief Retrieves the name of the current thread as a string.
   *
   * @return The thread name or a generated name if unavailable.
   */
  inline std::string getThreadName() {
    std::array<char, 16> buff{};
    getThreadName(buff);
    return buff.data();
  }

}  // namespace soralog::util
