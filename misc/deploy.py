#!/usr/bin/python3
#
# (C) Copyright 2021 Dominik Laton
#
# SPDX-License-Identifier: MIT
#
from prepare_iota_data import firmware, global_config, local_config
import argparse
import os
import json
import glob
import requests
import subprocess


def find_files(pattern, file_dir):
    file_list = None

    file_list = glob.glob(os.path.join(file_dir, pattern))

    return file_list


def get_chip_id(path):
    chip_id = None
    
    file_path = os.path.splitext(path)
    chip_id = file_path[1].replace(".", "")

    return chip_id


def upload_global_config(config_dir, iota_token, iota_url):
    upload_url = iota_url + "/api/v1/deploy/global_config"
    headers = {"Content-Type" : "application/json", "X-auth-token" : iota_token,
               "X-global-config-key" : ""}

    for global_config in find_files("global_config.json", config_dir):
        with open(global_config, "rb") as f:
            plaintext = f.read()

        try:
            j = json.loads(plaintext)
            headers["X-global-config-key"] = j["global_config_key"]
        except json.JSONDecodeError as e:
            exit(-1)

        r = requests.put(upload_url, headers=headers, data=open(global_config,
                                                                "rb"))
        if r.status_code == 201:
            print("Global config successfully deployed")
        else:
            print("Error deploying global config:")
            print(r.status_code)
            print(r.text)


def upload_local_config(config_dir, iota_token, iota_url):
    upload_url = iota_url + "/api/v1/deploy/local_config"
    headers = {"Content-Type" : "application/json", "X-auth-token" : iota_token,
               "X-chip-id" : ""}

    for local_config in find_files("config.json.*", config_dir):
        if get_chip_id(local_config) == None:
            continue
        headers["X-chip-id"] = get_chip_id(local_config)
        r = requests.put(upload_url, headers=headers, data=open(local_config,
                                                                "rb"))
        if r.status_code == 201:
            print("Local config for chip_id (" + get_chip_id(local_config) + ") successfully deployed")
        else:
            print("Error deploying local config for chip_id (" +
                  get_chip_id(local_config) + "):")
            print(r.status_code)
            print(r.text)
        

def upload_firmware(config_dir, iota_token, iota_url):
    upload_url = iota_url + "/api/v1/deploy/firmware"
    headers = {"Content-Type" : "text/plain", "X-auth-token" : iota_token,
               "X-firmware-version" : ""}

    for firmware in find_files("firmware*.sig", config_dir):
        with open(find_files("fw_version", config_dir)[0], "r") as f:
            headers["X-firmware-version"] = f.read()
        r = requests.put(upload_url, headers=headers, data=open(firmware,
                                                                "rb"))
        if r.status_code == 201:
            print("Frimware (" + headers["X-firmware-version"] + ") successfully deployed")
        elif r.status_code == 304:
            print("Frimware (" + headers["X-firmware-version"] + ") already up to date")
        else:
            print("Error deploying firmware(" +
                  headers["X-firmware-version"] + "):")
            print(r.status_code)
            print(r.text)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="bringup")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./server_data")
    parser.add_argument("--mapping-file", type=str, default="misc/mapping.json")
    args = parser.parse_args()

    try:
        iota_url = os.environ["OWSF_SERVER_URL"]
    except:
        print("Please specify url to owsf server.")
        exit(-1)

    try:
        iota_token = os.environ["IOTA_TOKEN"]
    except:
        print("Please specify token for writing to owsf server.")
        exit(-1)

    upload_global_config(os.path.join(args.data_dir), iota_token, iota_url)
    local_config(os.path.join(args.mapping_file), os.path.join(args.output_dir))
    upload_local_config(os.path.join(args.output_dir), iota_token, iota_url)
    firmware(os.path.join(args.output_dir))
    upload_firmware(os.path.join(args.output_dir), iota_token, iota_url)
