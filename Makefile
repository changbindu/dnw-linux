# author: Du, Changbin <changbin.du@gmail.com>

secbulk_src = `pwd`/src/secbulk
dnw_src = src/dnw

all: secbulk dnw

secbulk:
	make -C /lib/modules/`uname -r`/build M=$(secbulk_src) modules

dnw:
	make -C $(dnw_src)

install:
	make -C $(dnw_src) install
	make -C /lib/modules/`uname -r`/build M=$(secbulk_src) modules_install
	cp dnw.rules /etc/udev/rules.d/
	depmod

clean:
	make -C $(dnw_src) clean
	make -C /lib/modules/`uname -r`/build M=$(secbulk_src) clean

