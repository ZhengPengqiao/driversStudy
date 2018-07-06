KERN_DIR =  /usr/src/linux-headers-4.4.0-141-generic/
# KERN_DIR =  /usr/src/linux-headers-4.4.0-121-generic/
# KERN_DIR =  /usr/src/linux-headers-4.4.0-111-generic/
# KERN_DIR =  /usr/src/linux-headers-4.4.0-128-generic/

MODULE_NAME = myvivid

$(MODULE_NAME)-objs  := vivid-core.o vivid-ctrls.o vivid-vid-common.o\
		vivid-vid-cap.o vivid-kthread-cap.o vivid-tpg.o vivid-tpg-colors.o

obj-m	+= $(MODULE_NAME).o

all:
	make -C $(KERN_DIR) M=`pwd` modules
	objdump -D $(MODULE_NAME).ko > $(MODULE_NAME).dis

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	@rm -rf $(MODULE_NAME).dis
