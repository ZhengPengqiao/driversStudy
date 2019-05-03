ifneq ($(KERNELRELEASE),)

obj-m += hello_chdev.o


else

KNAME := $(shell uname -r)
KDIR := /usr/src/linux-headers-$(KNAME)/

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc chdev_userapp.c -o chdev_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf chdev_userapp

endif
