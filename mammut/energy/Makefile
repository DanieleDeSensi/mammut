.PHONY: all clean cleanall

proto = $()
cpp = $(patsubst %.cpp, %.o, $(wildcard *.cpp))

ifeq ($(REMOTE),true)
	proto = $(patsubst %.proto, %.pb.o, $(wildcard *.proto))
endif

all: $(proto) $(cpp)

#%.pb.cc: %.proto
#	$(PROTOBUF_PATH_BIN)/protoc --cpp_out=. $<
%.pb.o: %.proto
	$(PROTOBUF_PATH_BIN)/protoc --cpp_out=. $<
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $(patsubst %.proto, %.pb.cc, $<)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ *.pb.*
