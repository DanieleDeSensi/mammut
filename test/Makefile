.PHONY: all clean cleanall
TARGET=$(patsubst %.cpp, %, $(wildcard *.cpp))

all: $(TARGET)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< ./googletest/googletest/make/gtest_main.a $(INCS) -I./googletest/googletest/include $(LDFLAGS) $(LDLIBS) $(COVERAGE_LIBS)
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ 
	-rm -fr $(TARGET)
