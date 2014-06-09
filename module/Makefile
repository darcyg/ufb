# set your module name here
# provide the following files
#
# MODULENAME.c	the driver
# MODULENAME.h  the include file
# MODULENAME_test.c	the driver test program
# MODULENAME_test.sh	the driver test script
#
MODULENAME 	:= ufb
DEVICE_FILE := /dev/ufb
### should not need to change stuff below ######################


KDIR		:= /lib/modules/$(shell uname -r)/build
#KDIR		:= /exports/linux-2.6.12.2/
PWD		:= $(shell pwd)

CC		:= gcc -Wall

ifeq ($(KERNELRELEASE),)
all:	$(MODULENAME)_test
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
else
  obj-m	:= $(MODULENAME).o
endif

$(MODULENAME)_test: $(MODULENAME)_test.c
	$(CC) -g -o $(MODULENAME)_test $(MODULENAME)_test.c


load:
	insmod ./$(MODULENAME).ko
	chmod 0666 $(DEVICE_FILE)

unload:
	rmmod $(MODULENAME)

test: $(MODULENAME)_test
	./$(MODULENAME)_test $(DEVICE_FILE)

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
	rm -f $(MODULENAME)_test

