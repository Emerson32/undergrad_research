#!/bin/bash

# parse_dev_id.sh - Grabs the bus id for a given device
DEV_LABEL=$1

HEAD_STR="1-$(udevadm info --query=property --name=$DEV_LABEL | \
    grep ID_PATH= | \
    cut -d ':' -f 2
)"

TAIL_STR="$(udevadm info --query=property --name=$DEV_LABEL | \
    grep ID_PATH= | \
    cut -d ':' -f 3 | cut -d '-' -f 1
)"

echo $HEAD_STR:$TAIL_STR
