ifneq ($(KERNELRELEASE),)

obj-m += cameratest.o

xd_mipi-objs := \
	cameratest.o \

else

#KDIR := /home/zpq/WorkSpaces/IMX6-Android-4.4.2-BSP/myandroid_google_20131220/kernel_imx
KDIR := /usr/src/linux-headers-4.4.0-111-generic/

all:
	#make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(KDIR)/../prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi- modules
	make -C $(KDIR) M=$(PWD) modules
	
clean:
	make -C $(KDIR) M=$(PWD) clean

endif
