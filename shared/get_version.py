#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#

Import("env")

import re
import subprocess

gitversion = subprocess.check_output(["git", "-C", env.subst("$SRC_DIR"),
                                   "describe", "--always",
                                   "--dirty"]).strip().decode()
version = "VERSION='" + gitversion + "'"

env.Append(CPPDEFINES = [version])

if env.subst("PIOENV") == "release" :
    m = re.match(r"(.*?)(-g[0-9a-f]{7})?(-dirty)?$", gitversion)
    if m and len(m.groups("")[1]) == 0 and len(m.groups("")[2]) == 0:
        env.Replace(PROGNAME="firmware_%s" % gitversion)
