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
export CXXFLAGS              = -Wall -pedantic --std=c++11 -g
export MODULES               = cpufreq topology energy task 
export LDLIBS                = -lm -pthread -lrt -lmammut

.PHONY: all local remote demo clean cleanall install uninstall

all:
	$(MAKE) local
local:
	$(MAKE) -C mammut local
remote:
	$(eval LDLIBS += -lprotobuf-lite)
	$(eval CXXFLAGS += -DMAMMUT_REMOTE)
	$(MAKE) -C mammut remote
demo:
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
