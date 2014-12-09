CC                   = gcc
CXX                  = g++ 
LINK_OPT             = 
VERSION              = 
OPTIMIZE_FLAGS       = -O3 -finline-functions 
CXXFLAGS             = 
CFLAGS               = -Wall -g -pedantic 
LDFLAGS              = 
INCS                 = -I../
LIBS                 = 
INCLUDES             =
TARGET               = demo-energy

.PHONY: clean cleanall

all: $(TARGET)

%: %.cpp ../mammut/libmammut.a
	$(CXX) -o $@ $< $(INCS) -lm -lpthread -lrt  -L../mammut -lmammut -lprotobuf-lite  
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)

