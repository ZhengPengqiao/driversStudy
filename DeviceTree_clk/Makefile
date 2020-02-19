ifneq ($(KERNELRELEASE),)

obj-m += clk_1_of.o

obj-m += clk_4_of.o

# 编译时间
COMPILE_TIME = $(shell date +"%Y-%m-%d %H:%M:%S")
EXTRA_CFLAGS += -DCOMPILE_TIME="\"$(COMPILE_TIME)\""

# 编译时间
KO_VERSION = "KO.0.1"
EXTRA_CFLAGS += -DKO_VERSION="\"$(KO_VERSION)\""

else

KDIR := /home/zpq/WorkPlatfrom/IMX8-L4.9.51/linux-imx

all:
	rm ./image -rf
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

endif
