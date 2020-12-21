#!/usr/bin/python3
#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#
from datetime import datetime as date
import re
import subprocess

Import("env")

gitversion = subprocess.check_output(["git", "-C", env.subst("$SRC_DIR"),
                                   "describe", "--always",
                                   "--dirty"]).strip().decode()

dstr = date.now().strftime("'\"%Y-%m-%d_%T\"'")

vers = ("VERSION", "'\"" + gitversion + "\"'")
bdate = ("DATE", dstr)

env.Append(CPPDEFINES=[bdate, vers])

if env.subst("PIOENV") == "release" :
    m = re.match(r"(.*?)(-g[0-9a-f]{7})?(-dirty)?$", gitversion)
    if m and len(m.groups("")[1]) == 0 and len(m.groups("")[2]) == 0:
        env.Replace(PROGNAME="firmware_%s" % gitversion)
