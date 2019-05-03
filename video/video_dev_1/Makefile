ifneq ($(KERNELRELEASE),)

obj-m += cameratest.o

xd_mipi-objs := \
	cameratest.o \

else

KNAME := $(shell uname -r)
KDIR := /usr/src/linux-headers-$(KNAME)/

all:
	#make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(KDIR)/../prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- modules
	make -C $(KDIR) M=$(PWD) modules
	
clean:
	make -C $(KDIR) M=$(PWD) clean

endif
