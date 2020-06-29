INCL_DIR = ./includes
OBJ_DIR = ./objs
SRC_DIR = ./src

CC_CROSS = arm-linux-gnueabihf-gcc
CC = gcc
CROSSFLAGS = -Wall -g -I/usr/include -I$(INCL_DIR)
CFLAGS = -Wall -g -I$(INCL_DIR)

LDFLAGS = -L/usr/lib/arm-linux-gnueabihf

OBJS = $(OBJ_DIR)/key_packet.o
OBJS += $(OBJ_DIR)/bind_mgmt.o
OBJS += $(OBJ_DIR)/id_list.o
OBJS += $(OBJ_DIR)/pibox_unetlink.o

$(info objs: $(OBJS))

build_cross: $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC_CROSS) $(CROSSFLAGS) -c $< -o $@ 

# build: $(OBJS)

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
# 	$(CC) $(CFLAGS) -c $< -o $@ 

pibox_unetlink: $(OBJS)
	$(CC_CROSS) $(CROSSFLAGS) -o $@ $^ -ludev

clean:
	rm -rf pibox_unetlink $(OBJS)
