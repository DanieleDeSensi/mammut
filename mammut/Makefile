LDFLAGS              = -L. -L$(PROTOBUF_PATH_LIB)
LDLIBS               = -lm -pthread -lrt -lmammut -lprotobuf-lite 
INCS                 = -I../ -I$(PROTOBUF_PATH_INCLUDE)
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
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
mammut-server: $(TARGET) libmammut.a
	$(CXX) $(CXXFLAGS) -o mammut-server mammut-server.cpp $(INCS) $(LDFLAGS) $(LDLIBS)
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
