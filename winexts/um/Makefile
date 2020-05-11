winexts-y += ../main.o ../file_ops.o ../memutils.o ../utils.o ../communicate.o
obj-m += winexts.o
ccflags-y += -g -O0 -Wall #*-Wa,-a,-ad

all:
	make ARCH=um -C /lib/modules/5uml/build M=$(PWD) modules

clean:
	make ARCH=um -C /lib/modules/5uml/build M=$(PWD) clean
