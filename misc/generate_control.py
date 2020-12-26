#!/usr/bin/python3
#
# (C) Copyright 2020 Tillmann Heidsieck
#
# SPDX-License-Identifier: MIT
#
from subprocess import check_output
import argparse
import base64
import json
import nacl.secret
import nacl.utils
import os
import sys

def global_config(data_dir, output_dir):
    plaintext = None
    with open(os.path.join(data_dir, "global_config.js"), "rb") as f:
        plaintext = f.read()

    key = None
    try:
        j = json.loads(plaintext)
        key = base64.b64decode(j["global_config_key"])
    except json.JSONDecodeError as e:
        exit(-1)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    box = nacl.secret.SecretBox(key)
    with open(os.path.join(output_dir, "global_config.enc"), "wb") as f:
        f.write(box.encrypt(plaintext))


def local_config(mapping_file, output_dir):
    with open(mapping_file, "r") as f:
        j = json.load(f)

    for chip in j.keys():
        with open(j[chip]["sensor_config"], "r") as f:
            sensors = json.load(f)
        jf = {
            "config_version" : j[chip]["config_version"],
            "name" : j[chip]["name"],
            "sleep_time_s" : j[chip]["sleep_time_s"],
            "sensors" : sensors["sensors"],
        }
        with open(os.path.join(output_dir, "config.js.%s" % (chip)), "w") as f:
            f.write(json.dumps(jf, sort_keys=True, indent=4))


def firmware(src_dir, output_dir):
    version = check_output(["git", "-C", src_dir,
                            "describe", "--always", "--dirty"],
                           shell=False).strip().decode()
    filename = "firmware.sig.%s" % (version)
    fwjs = {
        "version" : version,
        "file" : filename,
    }

    with open(os.path.join(output_dir, "firmware.js"), "w") as f:
        f.write(json.dumps(fwjs, sort_keys=True, indent=4))

    with open(os.path.join(src_dir, ".pio", "build", "release", "firmware.sig"),
              "rb") as infile:
        with open(os.path.join(output_dir, filename), "wb") as outfile:
            outfile.write(infile.read())



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="prepare config files")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./server_data")
    parser.add_argument("--mapping-file", type=str, default="misc/mapping.js")
    args = parser.parse_args()

    global_config(args.data_dir, args.output_dir)
    local_config(args.mapping_file, args.output_dir)
    firmware(os.path.join(os.path.dirname(sys.argv[0]), ".."), args.output_dir)
