ifneq ($(KERNELRELEASE),)

obj-m += mem_test.o

else

KDIR := /usr/src/linux-headers-4.4.0-111-generic/

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc mem_userapp.c -o mem_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf mem_userapp

endif
