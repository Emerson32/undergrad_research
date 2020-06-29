obj-m += pibox_usb.o

all:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C /home/emerson/rpi_linux M=$(PWD) modules

clean:
	make -C /home/emerson/rpi_linux M=$(PWD) clean
