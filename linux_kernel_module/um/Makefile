# http://www.mjmwired.net/kernel/Documentation/kbuild/modules.txt

#obj-y += main.o
custom_module-y += ../utils.o ../memutils.o ../main.o ../communicate.o
obj-m += custom_module.o
ccflags-y += -Ofast -Wall #*-Wa,-a,-ad

all:
	make ARCH=um -C /lib/modules/`uname -r`-uml/build M=$(PWD) modules

clean:
	make ARCH=um -C /lib/modules/`uname -r`-uml/build M=$(PWD) clean
