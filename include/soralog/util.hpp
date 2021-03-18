/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SORALOG_UTIL
#define SORALOG_UTIL

#include <pthread.h>
#include <array>
#include <string>

namespace soralog::util {

  inline size_t getThreadNumber() {
    static std::atomic_size_t tid_counter = 0;
    static thread_local size_t tid = ++tid_counter;
    return tid;
  }

  inline void setThreadName(std::string_view name) {
    std::array<char, 16> buff{};
    memcpy(buff.data(), name.data(), std::min<size_t>(name.size(), 15));
#if defined(__linux__)
    pthread_setname_np(pthread_self(), buff.data());
#elif defined(__APPLE__)
    pthread_setname_np(buff.data());
#else
#warning \
    "Function setThreadName() is not implemented for current platform; An auto-generated name will be used instead"
#endif
  }

  inline void getThreadName(std::array<char, 16> &name) {
    static thread_local std::array<char, 16> thr_name{};
    static thread_local bool initialized = [&] {
#if defined(__linux__) or defined(__APPLE__)
      pthread_getname_np(pthread_self(), thr_name.data(), thr_name.size());
#else
#warning \
    "Function getThreadName() is not implemented for current platform; An auto-generated name will be used instead"
      auto generated = "Thread#" + std::to_string(getThreadNumber());
      memcpy(thr_name.data(), generated.data(),
             std::min(generated.size(), thr_name.size()));
#endif
      return true;
    }();
    name = thr_name;
  }

  inline std::string getThreadName() {
    std::array<char, 16> buff{};
    getThreadName(buff);
    return buff.data();
  }

}  // namespace soralog::util

#endif  // SORALOG_UTIL
