/**
 * Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soralog/util.hpp>

#include <algorithm>
#include <atomic>
#include <cstring>

#if !defined(_WIN32)
#include <pthread.h>
#endif

namespace soralog::util {

  size_t getThreadNumber() {
    static std::atomic_size_t tid_counter = 0;
    static thread_local size_t tid = ++tid_counter;
    return tid;
  }

  void setThreadName(std::string_view name) {
    std::array<char, 16> buff{};
    memcpy(buff.data(), name.data(), std::min<size_t>(name.size(), 15));

#if defined(__linux__)
    pthread_setname_np(pthread_self(), buff.data());
#elif defined(__APPLE__)
    pthread_setname_np(buff.data());
#elif !defined(_WIN32)
#warning \
    "Function setThreadName() is not implemented for this platform; \
    An auto-generated name will be used instead"
#endif
  }

  void getThreadName(std::array<char, 16> &name) {
    static thread_local std::array<char, 16> thr_name{};
    static thread_local bool initialized = [&] {
#if defined(__linux__) or defined(__APPLE__)
      pthread_getname_np(pthread_self(), thr_name.data(), thr_name.size());
#else
#if !defined(_WIN32)
#warning \
    "Function getThreadName() is not implemented for this platform; An auto-generated name will be used instead"
#endif
      auto generated = "Thread#" + std::to_string(getThreadNumber());
      memcpy(thr_name.data(),
             generated.data(),
             std::min(generated.size(), thr_name.size()));
#endif
      return true;
    }();
    name = thr_name;
  }

  std::string getThreadName() {
    std::array<char, 16> buff{};
    getThreadName(buff);
    return buff.data();
  }

}  // namespace soralog::util
