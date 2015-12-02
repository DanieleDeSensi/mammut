.PHONY: all clean

all:
	cd libusb-1.0.9 && ./configure
	make -C libusb-1.0.9
	make -C odroid-smartpower-linux
	make -C odroid-smartpower-linux lib

clean:
	make -C odroid-smartpower-linux clean
