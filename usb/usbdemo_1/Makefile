ifneq ($(KERNELRELEASE),)

obj-m += usb-skeleton.o

else

KNAME := $(shell uname -r)
KDIR := /usr/src/linux-headers-$(KNAME)/

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

endif
