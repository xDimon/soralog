[//]: # (
Copyright Soramitsu Co., 2021-2023
Copyright Quadrivium Co., 2023
All Rights Reserved
SPDX-License-Identifier: Apache-2.0
)

# Git Hooks

This folder _might_ contain some git-hooks to execute various checkup in soralog.

To activate presented (_and all future_) hooks just execute **once** shell-script [activate_hooks.sh](./activate_hooks.sh) from soralog project root path. 

## pre-commit

This hook checks that `clang-format` exists and has the recommended version (19).
If it does, the checkup is enabled.

Each staged C++ file (`.hpp` and `.cpp` extensions) gets processed by `clang-format`.

Checking CMake files with `cmake-format` is disabled in the hook for now.

Commit will be blocked while there are any differences between original files and `clang-format` output files.

## etc.

_Other hooks might be provided by maintainers in the future._

