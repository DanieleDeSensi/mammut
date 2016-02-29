TARGET               = offlining demo-topology idleLevels idleLevelsPower

.PHONY: all clean cleanall

all: $(TARGET)

%: %.cpp $(MAMMUTROOT)/mammut/libmammut.a
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCS) $(LDFLAGS) $(LDLIBS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)
