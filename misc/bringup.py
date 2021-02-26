#!/usr/bin/python3
#
# (C) Copyright 2021 Dominik Laton
#
# SPDX-License-Identifier: MIT
#
from prepare_iota_data import firmware
import argparse
import os
import subprocess


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="bringup")
    parser.add_argument("--data-dir", type=str, default="./data")
    parser.add_argument("--output-dir", type=str, default="./server_data")
    parser.add_argument("--mapping-file", type=str, default="misc/mapping.json")
    args = parser.parse_args()

    firmware(os.path.join(args.output_dir))
    subprocess.run(["misc/flash_firmware.sh", os.path.join(args.output_dir,
                                                         "firmware.sig")])
