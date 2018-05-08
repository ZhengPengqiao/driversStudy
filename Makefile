KERN_DIR =  /usr/src/linux-headers-4.4.0-121-generic/

obj-m	+= myvivi.o

all:
	make -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
