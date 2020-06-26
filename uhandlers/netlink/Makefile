CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g -I/usr/include
LDFLAGS = -L/usr/lib/arm-linux-gnueabihf

SRC_DIR = ./src
OBJ_DIR = ./objs

DEPS = $(SRC_DIR)/key_packet.h
DEPS += $(SRC_DIR)/bind_mgmt.h
DEPS+= $(SRC_DIR)/id_list.h

OBJS = $(OBJ_DIR)/key_packet.o
OBJS += $(OBJ_DIR)/bind_mgmt.o
OBJS += $(OBJ_DIR)/id_list.o
OBJS += $(OBJ_DIR)/pibox_unetlink.o

$(info objs: $(OBJS))

build: $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@ 

pibox_unetlink: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -ludev

clean:
	rm -rf pibox_unetlink $(OBJS)
