ifneq ($(KERNELRELEASE),)

obj-m += hello_chdev.o


else

KDIR := /usr/src/linux-headers-4.4.0-111-generic/

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc chdev_userapp.c -o chdev_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf chdev_userapp

endif
