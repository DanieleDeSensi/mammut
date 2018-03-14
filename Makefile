export PROTOBUF_PATH         = /usr/local
export PROTOBUF_PATH_LIB     = $(PROTOBUF_PATH)/lib
export PROTOBUF_PATH_INCLUDE = $(PROTOBUF_PATH)/include
export PROTOBUF_PATH_BIN    = $(PROTOBUF_PATH)/bin
export MAMMUT_PATH          = /usr/local
export MAMMUT_PATH_LIB      = $(MAMMUT_PATH)/lib
export MAMMUT_PATH_INCLUDE  = $(MAMMUT_PATH)/include/mammut
export MAMMUT_PATH_BIN      = $(MAMMUT_PATH)/bin

export CC                   = gcc
export CXX                  = g++
export OPTIMIZE_FLAGS       = -O3 -finline-functions -DAMESTER_ROOT=\"/tmp/amester\"
export CXXFLAGS             = -Wall -pedantic --std=c++11 $(TEST_FLAGS)
export MODULES              = cpufreq topology energy task 
export LDLIBS               = -lm -pthread -lrt -lmammut
export MAMMUTROOT           = $(realpath .)
export INCS                 = -I$(MAMMUTROOT) -I$(PROTOBUF_PATH_INCLUDE)
export LDFLAGS              = -L$(MAMMUTROOT)/mammut -L$(PROTOBUF_PATH_LIB)

.PHONY: all local remote demo clean cleanall install uninstall test cppcheck gcov develcheck

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
cppcheck:
# Cppcheck performed on Linux sources.
	cppcheck --xml --xml-version=2 --enable=warning,performance,information,style $(TEST_FLAGS) --error-exitcode=1  . --suppress="*:*.pb.cc" -imammut/external -itest 2> cppcheck-report.xml || (cat cppcheck-report.xml; exit 2) 
# Compiles and runs all the tests.
test:
	make cleanall
	make "MAMMUT_COVERAGE_FLAGS=-fprofile-arcs -ftest-coverage"
	cd test && ./installdep.sh 
	cd ..
	make "MAMMUT_COVERAGE_LIBS=-lgcov" -C test && cd test && ./runtests.sh
	cd ..
gcov:
	./test/gcov/gcov.sh
# Performs all the checks
develcheck:
# On Linux
	$(MAKE) "TEST_FLAGS=-D__linux__" cppcheck && $(MAKE) "TEST_FLAGS=-D__linux__" test && $(MAKE) gcov
# Reset
	$(MAKE) cleanall && $(MAKE)
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
