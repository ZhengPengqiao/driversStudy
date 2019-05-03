ifneq ($(KERNELRELEASE),)

# 编译时间
COMPILE_TIME = $(shell date +"%Y-%m-%d %H:%M:%S")
EXTRA_CFLAGS += -DCOMPILE_TIME="\"$(COMPILE_TIME)\""
# Kernel GIT版本号
KE_GIT_REVISION = $(shell git show -s --pretty=format:%h)
EXTRA_CFLAGS += -DKE_GIT_REVISION="\"$(KE_GIT_REVISION)\""
# ko GIT版本号
EXTRA_CFLAGS += -DKO_GIT_REVISION="\"$(KO_GIT_REVISION)\""


obj-m += sys_attr.o


else

KDIR := /usr/src/linux-headers-4.4.0-111-generic/

all:
	make -C $(KDIR) M=$(PWD) modules KO_GIT_REVISION=$(shell git show -s --pretty=format:%h)
	gcc sys_attr_userapp.c -o sys_attr_userapp
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf sys_attr_userapp

endif
