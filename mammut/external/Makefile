.PHONY: all clean

all:
ifeq ("$(wildcard ./libusb-1.0.9/libusb/.libs/libusb-1.0.a)","")
	cd libusb-1.0.9 && ./configure
	make -C libusb-1.0.9
endif
	
ifeq ("$(wildcard ./odroid-smartpower-linux/libsmartgauge.a)","")
	make -C odroid-smartpower-linux
	make -C odroid-smartpower-linux lib
endif

ifeq ("$(wildcard ./papi-5.5.1/src/libpapi.a)","")
	cd papi-5.5.1/src && ./configure && make -j
endif
clean:
	make -C odroid-smartpower-linux clean
