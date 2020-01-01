ifneq ($(KERNELRELEASE),)

obj-m += mem_test.o

else

# KDIR := /usr/src/linux-headers-$(shell uname -r)
KDIR := /home/zpq/WorkPlatfrom/IMX8/linux-imx

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) mem_userapp.c -o mem_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf mem_userapp

endif
