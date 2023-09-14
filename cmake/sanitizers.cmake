#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if(ASAN)
  print("Address Sanitizer is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/toolchain/flags/sanitize_address.cmake)
elseif(LSAN)
  print("Leak Sanitizer is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/toolchain/flags/sanitize_leak.cmake)
elseif(MSAN)
  print("Memory Sanitizer is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/toolchain/flags/sanitize_memory.cmake)
elseif(TSAN)
  print("Thread Sanitizer is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/toolchain/flags/sanitize_thread.cmake)
elseif(UBSAN)
  print("Undefined Behavior Sanitizer is enabled")
  include(${CMAKE_CURRENT_LIST_DIR}/toolchain/flags/sanitize_undefined.cmake)
endif()
