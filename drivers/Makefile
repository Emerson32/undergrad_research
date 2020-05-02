obj-m += hello-1.o

all:
	make ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /home/emerson/rpi_linux M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /home/emerson/rpi_linux M=$(PWD) clean
