obj-m +=mydriver.o

KDIR =//usr/src/linux-headers-4.6.3

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules	
clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order
