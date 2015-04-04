LDFLAGS              = -L../mammut -L$(PROTOBUF_PATH_LIB) 
LDLIBS               = -lm -pthread -lrt -lmammut -lprotobuf-lite
INCS                 = -I../ -I$(PROTOBUF_PATH_INCLUDE)
TARGET               = demo-energy demo-energy-extended demo-topology demo-task demo-frequency demo-voltage

.PHONY: clean cleanall

all: $(TARGET)

%: %.cpp ../mammut/libmammut.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)

