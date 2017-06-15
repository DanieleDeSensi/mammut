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
export OPTIMIZE_FLAGS        = -O3 -finline-functions -DAMESTER_ROOT=\"/tmp/amester\"
export CXXFLAGS              = -Wall -pedantic --std=c++11 $(TEST_FLAGS)
export MODULES               = cpufreq topology energy task 
export LDLIBS                = -lm -pthread -lrt -lmammut
export MAMMUTROOT           = $(realpath .)
export INCS                 = -I$(MAMMUTROOT) -I$(PROTOBUF_PATH_INCLUDE)
export LDFLAGS              = -L$(MAMMUTROOT)/mammut -L$(PROTOBUF_PATH_LIB)

.PHONY: all local remote demo clean cleanall install uninstall test

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
# Compiles and runs all the tests.
test:
	make cleanall
#We define __linux__ macro to be sure to execute the test for Linux environment
	make "TEST_FLAGS=-D__linux__"
	cd test && ./installdep.sh 
	cd ..
	make -C test && cd test && ./runtests.sh
	cd ..
clean: 
	$(MAKE) -C mammut clean
	$(MAKE) -C demo clean
	$(MAKE) -C test clean
cleanall:
	$(MAKE) -C mammut cleanall
	$(MAKE) -C demo cleanall
	$(MAKE) -C test cleanall
install:
	$(MAKE) -C mammut install
uninstall:
	$(MAKE) -C mammut uninstall
