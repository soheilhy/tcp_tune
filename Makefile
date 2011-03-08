obj-m += tcp_tune.o
tcp_tune-objs := module_core.o state_machine.o instructions.o sysctl_handler.o proc_handler.o

EXTRA_CFLAGS += -O2
#EXTRA_CFLAGS += -I$(LDDINC)

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

reinstall:
		sudo /sbin/rmmod tcp_tune
		sudo /sbin/insmod tcp_tune.ko 
	
install:
		sudo /sbin/insmod tcp_tune.ko
