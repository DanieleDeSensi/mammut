LDFLAGS              = -L../mammut -L$(PROTOBUF_PATH_LIB) 
LDLIBS               = -lm -pthread -lrt -lmammut -lprotobuf-lite
INCS                 = -I../
TARGET               = demo-energy demo-energy-extended

.PHONY: clean cleanall

all: $(TARGET)

%: %.cpp ../mammut/libmammut.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)

