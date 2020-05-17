winexts-y += ../communicate.o ../file_ops.o ../fork.o ../main.o ../mem_utils.o ../pattern_scanning.o ../ptrace_event_pid.o ../re_definitions.o ../utils.o
obj-m += winexts.o
ccflags-y += -g -O0 -Wall

all:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) modules

clean:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) clean
