#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

hunter_upload_password(
    # REPO_OWNER + REPO = https://github.com/forexample/hunter-cache
    REPO_OWNER "soramitsu"
    REPO "hunter-binary-cache"

    # USERNAME = warchant
    USERNAME "$ENV{GITHUB_HUNTER_USERNAME}"

    # PASSWORD = GitHub token saved as a secure environment variable
    PASSWORD "$ENV{GITHUB_HUNTER_TOKEN}"
)
