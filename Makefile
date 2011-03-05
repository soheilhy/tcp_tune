obj-m += tcp_tune.o
tcp_tune-m := instructions.o

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

reinstall:
		sudo /sbin/rmmod tcp_tune
		sudo /sbin/insmod tcp_tune.ko 
	
install:
		sudo /sbin/insmod tcp_tune.ko
