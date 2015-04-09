export PROTOBUF_PATH         = /usr/local
export PROTOBUF_PATH_LIB     = $(PROTOBUF_PATH)/lib
export PROTOBUF_PATH_INCLUDE = $(PROTOBUF_PATH)/include
export PROTOBUF_PATH_BIN     = $(PROTOBUF_PATH)/bin
export MAMMUT_PATH           = /usr/local
export MAMMUT_PATH_LIB       = $(MAMMUT_PATH)/lib
export MAMMUT_PATH_INCLUDE   = $(MAMMUT_PATH)/include/mammut
export MAMMUT_PATH_BIN       = $(MAMMUT_PATH)/bin

export CC                    = gcc
export CXX                   = g++
export OPTIMIZE_FLAGS        = -O3 -finline-functions 
export CXXFLAGS              = -Wall -g -pedantic
export MODULES               = cpufreq topology energy task 

.PHONY: all clean cleanall install uninstall

all:
	$(MAKE) -C mammut
	$(MAKE) -C demo
clean: 
	$(MAKE) -C mammut clean
	$(MAKE) -C demo clean
cleanall:
	$(MAKE) -C mammut cleanall
	$(MAKE) -C demo cleanall
install:
	$(MAKE) -C mammut install
uninstall:
	$(MAKE) -C mammut uninstall
