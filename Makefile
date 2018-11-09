obj-m := testModule.o
FLAGS = -W -Wall -g
CFLAGS_testModule.o = -Wframe-larger-than=2048
CC = gcc
KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:	commandLineUtility modules

modules:
	$(MAKE) $(CFLAGS_testModule.o) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules

commandLineUtility: commandLineUtility.o
	$(CC) $(FLAGS) -o commandLineUtility commandLineUtility.o

commandLineUtility.o: commandLineUtility.c
	$(CC) $(FLAGS) -c commandLineUtility.c

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf commandLineUtility