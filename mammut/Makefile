LDFLAGS              = -L. -L$(PROTOBUF_PATH_LIB)
LDLIBS               = -lm -pthread -lrt -lmammut -lprotobuf-lite 
INCS                 = -I../ -I$(PROTOBUF_PATH_INCLUDE)
TARGET               = utils.o communicator.o communicator-tcp.o
#HEADERS              = communicator.hpp communicator-tcp.hpp module.hpp utils.hpp cpufreq/cpufreq.hpp energy/energy.hpp topology/topology.hpp
MODULES              = cpufreq topology energy task
MODULES_OBJECTS      = $(MODULES:=/*.o)

.PHONY: clean cleanall install uninstall lib

all: $(TARGET)
	for MODULE in $(MODULES) ; do \
		make -C $$MODULE ; \
	done
	make lib
	make mammut-server
%.o: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
mammut-server: $(TARGET) libmammut.a
	$(CXX) $(CXXFLAGS) -o mammut-server mammut-server.cpp $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	for MODULE in $(MODULES) ; do \
		make -C $$MODULE clean; \
	done
	-rm -fr *.o *~
cleanall:
	for MODULE in $(MODULES) ; do \
		make -C $$MODULE cleanall; \
	done
	-rm -fr *.o *~ *.a mammut-server
lib: $(TARGET)
	ar rs libmammut.a $(TARGET) $(MODULES_OBJECTS)
install:
	mkdir -p $(MAMMUT_PATH_INCLUDE)
	find . -name '*.hpp' -exec cp --parents \{\} $(MAMMUT_PATH_INCLUDE) \;
	cp ./libmammut.a $(MAMMUT_PATH_LIB)/libmammut.a
	cp ./mammut-server $(MAMMUT_PATH_BIN)/mammut-server
uninstall:
	rm -r $(MAMMUT_PATH_INCLUDE)/*
	rm $(MAMMUT_PATH_LIB)/libmammut.a
	rm $(MAMMUT_PATH_BIN)/mammut-server
