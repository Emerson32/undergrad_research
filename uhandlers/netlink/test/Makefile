INCL_DIR = ../includes
SRC_DIR = ../src
OBJ_DIR = ../objs

CC = gcc
CFLAGS = -Wall -g -I$(INCL_DIR)

IDL_OBJS = $(OBJ_DIR)/id_list.o test_id_list.o

test_id_list: test_id_list.c
	$(CC) $(CFLAGS) -c $< -o $@.o
	$(CC) $(CFLAGS) -o $@ $(IDL_OBJS)

clean:
	rm -rf test_id_list test_id_list.o
