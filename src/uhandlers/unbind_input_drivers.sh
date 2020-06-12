#!/bin/bash
#
# unbind_input_drivers.sh - Temporarily unbinds device drivers from all input
#                           devices on the host machine

SCRIPT_DIR="/home/pi/scripts/pibox"

# An associative array that creates a 1-1 relationship between the input device id
# with the device label
declare -A input_devs

# Iterate through each input event* device and parse the device id
for input_dev in $(ls -p /dev/input | grep -v /); do
    if [ $input_dev != "mice" ]; then
        input_devs[$input_dev]=$("$SCRIPT_DIR/parse_dev_id.sh" "input/$input_dev")
    fi
done

# for key in "${input_devs[@]}"; do echo $key; done

# Unbind each of the drivers from the input devices
for id in "${input_devs[@]}"; do
    echo $id > /sys/bus/usb/drivers/usbhid/unbind
done

sleep 5

# Rebind each of the drivers to the input devices
for id in "${input_devs[@]}"; do
    echo $id > /sys/bus/usb/drivers/usbhid/bind
done

exit 0