.PHONY: all clean cleanall

all: $(patsubst %.proto, %.pb.o, $(wildcard *.proto)) $(patsubst %.cpp, %.o, $(wildcard *.cpp))

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
