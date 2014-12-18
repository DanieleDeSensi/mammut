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

.PHONY: clean cleanall install uninstall

all: $(TARGET)
	make -C mammut
	make -C demo
clean: 
	make -C mammut clean
	make -C demo clean
cleanall:
	make -C mammut cleanall
	make -C demo cleanall
install:
	make -C mammut install
uninstall:
	make -C mammut uninstall
