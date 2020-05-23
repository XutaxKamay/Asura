TASKLIST_LOCK_ADDR=$(shell cat /boot/System.map-`uname -r` | grep ' tasklist_lock' | cut -d ' ' -f1)
CSS_SET_LOCK_ADDR=$(shell cat /boot/System.map-`uname -r` | grep ' css_set_lock' | cut -d ' ' -f1)
CPU_RUNQUEUES_ADDR=$(shell cat /boot/System.map-`uname -r` | grep ' runqueues' | cut -d ' ' -f1)

winexts-y += communicate.o file_ops.o fork.o main.o mem_utils.o pattern_scanning.o re_definitions.o utils.o
obj-m += winexts.o
ccflags-y += -g -Ofast -Wall -DCSS_SET_LOCK_ADDR=0x$(CSS_SET_LOCK_ADDR) -DTASKLIST_LOCK_ADDR=0x$(TASKLIST_LOCK_ADDR) -DCPU_RUNQUEUES_ADDR=0x$(CPU_RUNQUEUES_ADDR)

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
