# ISL79987 MIPI driver for Linux3.14.52 and Android-6.0.1

ifneq ($(KERNELRELEASE),)

obj-m += gpio.o

xd_mipi-objs := \
	gpio.o

# 编译时间
COMPILE_TIME = $(shell date +"%Y-%m-%d %H:%M:%S")
EXTRA_CFLAGS += -DCOMPILE_TIME="\"$(COMPILE_TIME)\""
# Kernel GIT版本号
KE_GIT_REVISION = $(shell git show -s --pretty=format:%h)
EXTRA_CFLAGS += -DKE_GIT_REVISION="\"$(KE_GIT_REVISION)\""
# ko GIT版本号
EXTRA_CFLAGS += -DKO_GIT_REVISION="\"$(KO_GIT_REVISION)\""


else

KDIR := /home/zpq/WorkSpace/worklibrary/tamsong/tsh_kernel


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules  KO_GIT_REVISION=$(shell git show -s --pretty=format:%h)
	$(CC) gpio_userapp.c -o gpio_userapp
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -rf gpio_userapp

endif
