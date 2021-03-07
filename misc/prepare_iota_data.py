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
import requests
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

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

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


def firmware(output_dir):
    fw_dl_link = None

    r = requests.get("https://api.github.com/repos/owsf/owsf-firmware/releases/latest")
    if r.status_code != 200:
        sys.exit(-1)

    response = r.json()
    for asset in response["assets"]:
        if "firmware.sig" in asset["browser_download_url"]:
            fw_dl_link = asset["browser_download_url"]

    if fw_dl_link == None:
        sys.exit(-1)

    r = requests.get(fw_dl_link)
    if r.status_code != 200:
        sys.exit(-1)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    with open(os.path.join(output_dir, "firmware.sig"), "w+b") as f:
        f.write(r.content)

    with open(os.path.join(output_dir, "fw_version"), "w") as f:
        f.write(response["tag_name"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="prepare config files")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./server_data")
    parser.add_argument("--mapping-file", type=str, default="misc/mapping.json")
    args = parser.parse_args()

    global_config(args.data_dir, args.output_dir)
    local_config(args.mapping_file, args.output_dir)
    firmware(os.path.join(args.output_dir))
