winexts-y += communicate.o file_ops.o fork.o main.o mem_utils.o pattern_scanning.o re_definitions.o utils.o switch_to.o hooks.o schedule_hooks.o
obj-m += winexts.o
ccflags-y += -g -Ofast -Wall

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
