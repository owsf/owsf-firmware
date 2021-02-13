#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#

from OpenSSL import crypto
import os

Import("env")

print("Prepare public key")

default = "misc/public.key"
pubkey_data = ""
try:
    pubkey_data = os.environ["FIRMWARE_PUBLIC_KEY"]
except:
    with open(default, "r") as f:
        pubkey_data = f.read()

if not pubkey_data:
    env.Append(CPPDEFINES = ["-DSIGNED_UPDATES=0"])
    exit(0)

try:
    tmp = crypto.load_publickey(crypto.FILETYPE_PEM, pubkey_data)
except:
    exit(0)

with open("include/signing_pubkey.h", "w") as f:
    f.write('#ifndef _GENERATED_PUBKEY_H_\n')
    f.write('#define _GENERATED_PUBKEY_H_\n')
    f.write('#include <Arduino.h>\n')
    f.write('#include "pubkey.h"\n\n')
    f.write('const char pubkey[] PROGMEM = R"EOF(\n')
    f.write(pubkey_data)
    f.write(')EOF";\n')
    f.write('#endif')
