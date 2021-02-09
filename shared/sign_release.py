#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#

from OpenSSL import crypto
import base64
import os
import struct

Import("env", "projenv")

def sign_binary(source, target, env):
    # If not set we let it crash
    try:
        pkey = os.environ["FIRMWARE_SIGNING_PKEY"]
    except:
        print("No signing key specified")
        return 0
    
    if os.path.exists(pkey):
        try:
            with open(pkey, "r") as f:
                pkey = f.read()
        except:
            print("Signing key not found")
            return 0
    
    try:
        pk = crypto.load_privatekey(crypto.FILETYPE_PEM, pkey)
    except:
        print("Failed to load signing key")
        return 0
    
    with open(env.subst("$BUILD_DIR/${PROGNAME}.bin"), "rb") as f:
        data = f.read()
    
    sig = crypto.sign(pk, data, "sha256")

    with open(env.subst("$BUILD_DIR/${PROGNAME}.sig"), "wb") as f:
        f.write(data)
        f.write(sig)
        f.write(struct.pack("I", len(sig)))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", sign_binary)
