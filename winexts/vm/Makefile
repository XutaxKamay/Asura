winexts-y += ../main.o ../file_ops.o ../memutils.o ../utils.o ../communicate.o
obj-m += winexts.o
ccflags-y += -g -Wall #*-Wa,-a,-ad

all:
	make ARCH=um -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) modules

clean:
	make ARCH=um -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) clean
