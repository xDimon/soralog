/**
* Copyright Soramitsu Co., 2021-2023
 * Copyright Quadrivium Co., 2023
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

namespace soralog {

  /**
   * @brief Dummy symbol to prevent empty translation unit warnings.
   *
   * Some compilers issue warnings when a translation unit does not contain
   * any symbols. This extern constant ensures the object file is not empty.
   */
  extern const char *const __library_name;  // NOLINT

  /**
   * @brief Name of the library, "soralog".
   *
   * A function, unlike the constant above, is exported from soralog.dll on
   * Windows, so the DLL is not left without exports.
   */
  const char *libraryName();

}  // namespace soralog