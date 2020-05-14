winexts-y += main.o file_ops.o memutils.o utils.o communicate.o
obj-m += winexts.o
ccflags-y += -g -Ofast -Wall #*-Wa,-a,-ad

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
