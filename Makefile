# author: Du, Changbin <changbin.du@gmail.com>

driver_src = `pwd`/src/driver
dnw_src = src/dnw

all: driver dnw

driver:
	make -C /lib/modules/`uname -r`/build M=$(driver_src) modules

dnw:
	make -C $(dnw_src)

install: all
	make -C $(dnw_src) install
	make -C /lib/modules/`uname -r`/build M=$(driver_src) modules_install
	cp dnw.rules /etc/udev/rules.d/
	depmod

clean:
	make -C $(dnw_src) clean
	make -C /lib/modules/`uname -r`/build M=$(driver_src) clean

