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
import re
import sys

def global_config(data_dir, output_dir):
    plaintext = None
    with open(os.path.join(data_dir, "global_config.json"), "rb") as f:
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
        sensors = {}
        sensors["sensors"] = []
        for config in j[chip]["sensor_config"]:
            with open(config, "r") as f:
                sensors_buf = json.load(f)
                for sensor in sensors_buf["sensors"]:
                    sensors["sensors"].append(sensor)
        jf = {
            "config_version" : j[chip]["config_version"],
            "device_name" : j[chip]["device_name"],
            "sleep_time_s" : j[chip]["sleep_time_s"],
            "sensors" : sensors["sensors"],
        }
        with open(os.path.join(output_dir, "config.json.%s" % (chip)), "w") as f:
            f.write(json.dumps(jf, sort_keys=True, indent=4))


def firmware(src_dir, output_dir):
    version = check_output(["git", "-C", src_dir,
                            "describe", "--always", "--dirty"],
                           shell=False).strip().decode()

    m = re.match(r"(.*?)(-g[0-9a-f]{7})?(-dirty)?$", version)

    filename = "firmware.sig.%s" % (version)
    if m and len(m.groups("")[2]) == 0:
       filename = "firmware.sig.%s" % m.groups()[0].replace("-", ".")

    fwjs = {
        "version" : version if not m else m.groups()[0].replace("-","."),
        "file" : filename,
    }

    with open(os.path.join(output_dir, "firmware.json"), "w") as f:
        f.write(json.dumps(fwjs, sort_keys=True, indent=4))

    ffile = "firmware.sig"
    if m and len(m.groups("")[2]) == 0:
       ffile = "firmware.%s.sig" % m.groups()[0].replace("-", ".")

    ffile_path = os.path.join(src_dir, ".pio", "build", "release", ffile)
    with open(ffile_path, "rb") as infile:
        with open(os.path.join(output_dir, filename), "wb") as outfile:
            outfile.write(infile.read())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="prepare config files")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./server_data")
    parser.add_argument("--mapping-file", type=str, default="misc/my_sensors/mapping.json")
    args = parser.parse_args()

    global_config(args.data_dir, args.output_dir)
    local_config(args.mapping_file, args.output_dir)
    #firmware(os.path.join(os.path.dirname(sys.argv[0]), ".."), args.output_dir)
