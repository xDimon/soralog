#!/bin/bash -xe
#
# Copyright Soramitsu Co., 2021-2023
# Copyright Quadrivium Co., 2023
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

buildDir=$1
token=$2

which git


if [ -z "$buildDir" ]; then
    echo "buildDir is empty"
    exit 1
fi

if [ -z "$token" ]; then
    echo "token arg is empty"
    exit 2
fi

bash <(curl -s https://codecov.io/bash) -s $buildDir -t $token
