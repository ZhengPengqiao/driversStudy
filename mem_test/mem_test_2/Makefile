ifneq ($(KERNELRELEASE),)

obj-m += mem_test.o

else

KNAME := $(shell uname -r)
KDIR := /usr/src/linux-headers-$(KNAME)/

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc mem_userapp.c -o mem_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf mem_userapp

endif
