#
# (C) Copyright 2021 Dominik Laton
#
# SPDX-License-Identifier: MIT
#
Import("env")

env.AddCustomTarget(
    "deploy",
    None,
    "misc/deploy.py"
)

env.AddCustomTarget(
    "bringup",
    None,
    "misc/bringup.py"
)
