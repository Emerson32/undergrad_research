#!/bin/bash

# unbind_driver.sh - Simple script responsible for unbinding a given driver
#                    from a given device label, e.g. /dev/sda

if [ $# -ne 2 ]; then
    exit 1
fi

TARGET_DRIVER=$1
DEV_LABEL=$2
DEV_ID=$(parse_dev_id.sh $DEV_LABEL)

echo $DEV_ID > /sys/bus/usb/drivers/$TARGET_DRIVER/unbind
