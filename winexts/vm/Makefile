winexts-y += ../communicate.o ../file_ops.o ../fork.o ../main.o ../mem_utils.o ../pattern_scanning.o ../re_definitions.o ../utils.o ../switch_to.o ../hooks.o ../schedule_hooks.o
obj-m += winexts.o
ccflags-y += -g -O1 -Wall
all:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) modules

clean:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) clean
