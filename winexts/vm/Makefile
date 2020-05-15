winexts-y += ../main.o ../file_ops.o ../memutils.o ../utils.o ../communicate.o
obj-m += winexts.o
ccflags-y += -g -O1 -Wall #*-Wa,-a,-ad

all:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) modules

clean:
	make -C /home/kamay/vmshare/linux-liquorix-5.6 M=$(PWD) clean
