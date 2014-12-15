CC                   = gcc
CXX                  = g++ 
LINK_OPT             = 
VERSION              = 
OPTIMIZE_FLAGS       = -O3 -finline-functions
CXXFLAGS             = 
CFLAGS               = -Wall -g -pedantic
LDFLAGS              = 
INCS                 = -I../../ -I $(PROTOBUF_PATH)/include
LIBS                 = 
INCLUDES             =
TARGETCC             = cpufreq-remote.pb.cc
TARGETOBJ            = cpufreq-remote.pb.o cpufreq.o cpufreq-linux.o cpufreq-remote.o 

.PHONY: clean cleanall

all: $(TARGETCC) $(TARGETOBJ)

%.pb.cc: %.proto
	protoc --cpp_out=. $<
%.pb.o: %.pb.cc %.pb.h
	$(CXX) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $< $(CFLAGS)
%.o: %.cpp *.hpp
	$(CXX) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $< $(CFLAGS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ *.pb.*
