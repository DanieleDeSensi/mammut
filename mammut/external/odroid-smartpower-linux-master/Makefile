CC       ?= gcc
CFLAGS   ?= -Wall -g -fpic

CXX      ?= g++
CXXFLAGS ?= -Wall -g -fpic -std=c++11

LDFLAGS  ?= -Wall -g

COBJS     = hid.o 
CPPOBJS   = smartgauge.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS_USB  = `pkg-config libusb-1.0 --libs` -lrt -lpthread
LIBS      = $(LIBS_USB)
INCLUDES  = -I ../libusb-1.0.9/

all: $(OBJS)

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) $< -o $@

lib: $(OBJS)
	ar rvs libsmartgauge.a smartgauge.o hid.o

clean:
	rm -f $(OBJS) 

.PHONY: clean libs
