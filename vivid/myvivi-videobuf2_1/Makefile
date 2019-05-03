KNAME := $(shell uname -r)
KERN_DIR := /usr/src/linux-headers-$(KNAME)/

MODULE_NAME = myvivid

$(MODULE_NAME)-objs  := vivid-core.o vivid-ctrls.o vivid-vid-common.o vivid-vbi-gen.o \
		vivid-vid-cap.o vivid-vid-out.o vivid-kthread-cap.o vivid-kthread-out.o \
		vivid-radio-rx.o vivid-radio-tx.o vivid-radio-common.o \
		vivid-rds-gen.o vivid-sdr-cap.o vivid-vbi-cap.o vivid-vbi-out.o \
		vivid-osd.o vivid-tpg.o vivid-tpg-colors.o

obj-m	+= $(MODULE_NAME).o

all:
	make -C $(KERN_DIR) M=`pwd` modules
	objdump -D $(MODULE_NAME).ko > $(MODULE_NAME).dis

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	@rm -rf $(MODULE_NAME).dis
