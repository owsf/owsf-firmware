#!/bin/python
#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#

from OpenSSL import crypto
import os

Import("projenv")

s = 1
try:
    pkey = os.environ["FIRMWARE_SIGNING_PKEY"]
except:
    s = 0

if s:
    if os.path.exists(pkey):
        try:
            with open(pkey, "r") as f:
                pkey = f.read()
        except:
            s = 0
if s:
    try:
        pk = crypto.load_privatekey(crypto.FILETYPE_PEM, pkey)
    except:
        s = 0

signing = "-DSIGNED_UPDATES=%d" % s
projenv.Append(CPPDEFINES = [signing])
