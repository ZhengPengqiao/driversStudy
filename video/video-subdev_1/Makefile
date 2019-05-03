KNAME := $(shell uname -r)
KDIR := /usr/src/linux-headers-$(KNAME)/

MODULE_NAME = myvivid

$(MODULE_NAME)-objs := myvivi.o

obj-m	+= $(MODULE_NAME).o

all:
	make -C $(KDIR) M=`pwd` modules
	objdump -D $(MODULE_NAME).ko > $(MODULE_NAME).dis

clean:
	make -C $(KDIR) M=`pwd` modules clean
	@rm -rf $(MODULE_NAME).dis
