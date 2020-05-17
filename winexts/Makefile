winexts-y += communicate.o file_ops.o fork.o main.o mem_utils.o pattern_scanning.o ptrace_event_pid.o re_definitions.o utils.o
obj-m += winexts.o
ccflags-y += -g -Ofast -Wall

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
