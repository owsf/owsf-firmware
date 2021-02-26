#!/bin/bash
#
# (C) Copyright 2021 Dominik Laton
#
# SPDX-License-Identifier: MIT
#

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 <path to firmware.sig>. Exiting now"
	exit 1
fi
ESPTOOL=$(find $HOME/.platformio -wholename \*tool-esptoolpy/esptool.py)
FW="$1"

# Config for esptool
export ESPTOOL_PORT="/dev/ttyUSB0"
export ESPTOOL_BAUD="921600"

python3 $ESPTOOL --before default_reset --after hard_reset --chip esp8266 write_flash 0x0 $FW

exit $?
