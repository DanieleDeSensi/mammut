INCS                 = -I../../ -I$(PROTOBUF_PATH_INCLUDE)
TARGETCC             = topology-remote.pb.cc
TARGETOBJ            = topology-remote.pb.o topology.o topology-linux.o topology-remote.o 

.PHONY: all clean cleanall

all: $(TARGETCC) $(TARGETOBJ)

%.pb.cc: %.proto
	$(PROTOBUF_PATH_BIN)/protoc --cpp_out=. $<
%.pb.o: %.pb.cc %.pb.h
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
%.o: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ *.pb.*
