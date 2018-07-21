MODULES_SOURCES      = $(MODULES:=/*.cpp) $(MODULES:=/*.proto) $(MODULES:=/*.cc)
GENERAL_SOURCES      = *.cpp
MODULES_OBJECTS      = $(MODULES:=/*.o)
GENERAL_OBJECTS      = mammut.o utils.o communicator.o communicator-tcp.o
PYBIND_OBJECTS       = mammut_pybind.o
OBJECTS              = $(GENERAL_OBJECTS) $(MODULES_OBJECTS) 

.PHONY: all local remote clean cleanall install uninstall lib python mammut-server

all:
	$(MAKE) local
local:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE ; \
	done
	$(MAKE) -C external
	$(MAKE) lib
remote:
	$(eval export REMOTE = true)
	$(MAKE) local
	$(MAKE) mammut-server
%_pybind.o: %_pybind.cpp *.hpp
	$(CXX) $(CXXFLAGS) -shared `python3 -m pybind11 --includes`  $(INCS) $(OPTIMIZE_FLAGS) $< -o $(subst _pybind.o,,$@)`python3-config --extension-suffix` $(OBJECTS) -L$(MAMMUTROOT)/mammut/external/odroid-smartpower-linux -lsmartgauge  $(MAMMUTROOT)/mammut/external/libusb-1.0.9/libusb/.libs/libusb-1.0.so.0.1.0 
%.o: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(MAMMUT_COVERAGE_FLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
mammut-server: $(OBJECTS) libmammut.a
	$(CXX) $(CXXFLAGS) -o mammut-server mammut-server.cpp $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE clean; \
	done
	$(MAKE) -C external clean
	-rm -fr *.o *~
	-rm -fr *`python3-config --extension-suffix`
cleanall:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE cleanall; \
	done
	$(MAKE) -C external clean
	-rm -fr *.o *~ *.a mammut-server
	-rm -fr *`python3-config --extension-suffix`
lib: $(OBJECTS)
	rm -rf libmammut.a
	ar rs libmammut.a $(OBJECTS) 
	ar -M < buildlib.ar 
python: $(PYBIND_OBJECTS)

install:
	mkdir -p $(MAMMUT_PATH_INCLUDE)
	find . -name '*.hpp' -exec cp --parents \{\} $(MAMMUT_PATH_INCLUDE) \;
	mkdir -p $(MAMMUT_PATH_LIB)
	cp ./libmammut.a $(MAMMUT_PATH_LIB)/libmammut.a
	mkdir -p $(MAMMUT_PATH_BIN)
	cp ./mammut-server $(MAMMUT_PATH_BIN)/mammut-server
uninstall:
	rm -r $(MAMMUT_PATH_INCLUDE)/*
	rm $(MAMMUT_PATH_LIB)/libmammut.a
ifeq ("$(wildcard $(MAMMUT_PATH_BIN)/mammut-server)","")
	rm $(MAMMUT_PATH_BIN)/mammut-server
endif
