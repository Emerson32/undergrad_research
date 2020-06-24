#ifndef BIND_MGMT_H
#define BIND_MGMT_H

// TODO: Implement the driver unbinding operation

/*
* Uses regex to find the bus id from the given device path
*/
void find_bus_id(char *result, char *dev_path);

/*
* Writes the bus id to the UNBIND_PATH,
* effectively unbinding the HID driver from the respective device
*/
int write_to_sys(FILE *file, char* bus_id);

#endif  // BIND_MGMT_H