#ifndef BIND_MGMT_H
#define BIND_MGMT_H

#define UNBIND_PATH "/sys/bus/usb/drivers/usbhid/unbind"
#define BIND_PATH   "/sys/bus/usb/drivers/usbhid/bind"

/*
*   Searches for a substring within dev_path that has the bus ID
*   format and copies the string into the result param.
*/
int find_bus_id(char *result, char *dev_path);

/*
* Writes the bus id to either BIND_PATH OR UNBIND_PATH,
* effecitvely binding or unbinding the HID driver from the device.
*/
int write_to_sys(FILE **file, char **bus_id);

#endif  // BIND_MGMT_H