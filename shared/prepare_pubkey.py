#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#

from OpenSSL import crypto
import os

Import("env", "projenv")

print("Prepare public key")

default = "misc/public.key"
try:
    pubkey = os.environ["FIRMWARE_PUBLIC_KEY"]
except:
    pubkey = default

if not os.path.exists(pubkey):
    projenv.Append(CPPDEFINES = ["-DSIGNED_UPDATES=0"])
    os.exit(0)

pubkey_data = ""
with open(pubkey, "r") as f:
    pubkey_data = f.read()

try:
    tmp = crypto.load_publickey(crypto.FILETYPE_PEM, pubkey_data)
except:
    os.exit(0)

with open(projenv.subst("$BUILD_DIR/pubkey.cpp"), "w") as f:
    f.write('#include "pubkey.h"\n\n')
    f.write('const char pubkey[] PROGMEM = R"EOF(\n')
    f.write(pubkey_data)
    f.write('EOF)";')
    env.Append(EXTRA_LIBRARY_DIRS=["$BUILD_DIR"])
