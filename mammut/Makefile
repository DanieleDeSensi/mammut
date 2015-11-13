LDFLAGS              = -L. -L$(PROTOBUF_PATH_LIB)
export INCS          = -I$(realpath ..) -I$(PROTOBUF_PATH_INCLUDE)
MODULES_SOURCES      = $(MODULES:=/*.cpp) $(MODULES:=/*.proto) $(MODULES:=/*.cc)
GENERAL_SOURCES      = *.cpp
MODULES_OBJECTS      = $(MODULES:=/*.o)
GENERAL_OBJECTS      = utils.o communicator.o communicator-tcp.o mammut.o

.PHONY: all local remote clean cleanall install uninstall lib mammut-server

all:
	$(MAKE) local
local:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE ; \
	done
	$(MAKE) lib
remote:
	$(eval export REMOTE = true)
	$(MAKE) local
	$(MAKE) mammut-server
%.o: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
mammut-server: $(GENERAL_OBJECTS) $(MODULES_OBJECTS) libmammut.a
	$(CXX) $(CXXFLAGS) -o mammut-server mammut-server.cpp $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE clean; \
	done
	-rm -fr *.o *~
cleanall:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE cleanall; \
	done
	-rm -fr *.o *~ *.a mammut-server
lib: $(GENERAL_OBJECTS) $(MODULES_OBJECTS)
	ar rs libmammut.a $(GENERAL_OBJECTS) $(MODULES_OBJECTS)
install:
	mkdir -p $(MAMMUT_PATH_INCLUDE)
	find . -name '*.hpp' -exec cp --parents \{\} $(MAMMUT_PATH_INCLUDE) \;
	cp ./libmammut.a $(MAMMUT_PATH_LIB)/libmammut.a
ifneq ("$(wildcard mammut-server)","")
	cp ./mammut-server $(MAMMUT_PATH_BIN)/mammut-server
endif
uninstall:
	rm -r $(MAMMUT_PATH_INCLUDE)/*
	rm $(MAMMUT_PATH_LIB)/libmammut.a
ifneq ("$(wildcard $(MAMMUT_PATH_BIN)/mammut-server)","")
	rm $(MAMMUT_PATH_BIN)/mammut-server
endif
