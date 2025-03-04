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
  const char *__library_name;  // NOLINT

}  // namespace soralog