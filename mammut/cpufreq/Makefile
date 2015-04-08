INCS                 = -I../../ -I$(PROTOBUF_PATH_INCLUDE)
TARGETCC             = cpufreq-remote.pb.cc
TARGETOBJ            = cpufreq-remote.pb.o cpufreq.o cpufreq-linux.o cpufreq-remote.o 

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
