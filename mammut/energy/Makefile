INCS                 = -I../../ -I$(PROTOBUF_PATH_INCLUDE)
TARGETCC             = energy-remote.pb.cc
TARGETOBJ            = energy-remote.pb.o energy.o energy-linux.o energy-remote.o 

.PHONY: clean cleanall

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
