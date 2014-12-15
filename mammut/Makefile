PROTOBUF_PATH        = /usr/local
export PROTOBUF_PATH
MAMMUT_PATH          = /usr/local
MAMMUT_PATH_LIB      = $(MAMMUT_PATH)/lib
MAMMUT_PATH_INCLUDE  = $(MAMMUT_PATH)/include/mammut
MAMMUT_PATH_BIN      = $(MAMMUT_PATH)/bin
CC                   = gcc
CXX                  = g++ 
LINK_OPT             = 
VERSION              = 
OPTIMIZE_FLAGS       = -O3 -finline-functions 
CXXFLAGS             = 
CFLAGS               = -Wall -g -pedantic
LDFLAGS              = -lm -lpthread -lrt -L. -lmammut -L$(PROTOBUF_PATH)/lib -lprotobuf-lite
INCS                 = -I ../ -I $(PROTOBUF_PATH)/include
LIBS                 = 
INCLUDES             =
TARGET               = utils.o communicator.o communicator-tcp.o
HEADERS              = communicator.hpp communicator-tcp.hpp module.hpp utils.hpp cpufreq/cpufreq.hpp energy/energy.hpp topology/topology.hpp

.PHONY: clean cleanall install uninstall lib

all: $(TARGET)
	make -C cpufreq
	make -C topology
	make -C energy
	make lib
	make mammut-server
%.o: %.cpp *.hpp
	$(CXX) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $< $(CFLAGS)
mammut-server: $(TARGET) libmammut.a
	$(CXX) -o mammut-server mammut-server.cpp $(INCS) $(LDFLAGS)
clean: 
	make -C cpufreq clean
	make -C topology clean
	make -C energy clean
	-rm -fr *.o *~
cleanall:
	make -C cpufreq cleanall
	make -C topology cleanall
	make -C energy cleanall
	-rm -fr *.o *~ *.a mammut-server
lib: $(TARGET)
	ar rs libmammut.a $(TARGET) cpufreq/*.o topology/*.o energy/*.o
install:
	mkdir -p $(MAMMUT_PATH_INCLUDE)
	find . -name '*.hpp' -exec cp --parents \{\} $(MAMMUT_PATH_INCLUDE) \;
	cp ./libmammut.a $(MAMMUT_PATH_LIB)/libmammut.a
	cp ./mammut-server $(MAMMUT_PATH_BIN)/mammut-server
uninstall:
	rm -r $(MAMMUT_PATH_INCLUDE)/*
	rm $(MAMMUT_PATH_LIB)/libmammut.a
	rm $(MAMMUT_PATH_BIN)/mammut-server
