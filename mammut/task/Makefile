INCS                 = -I../../ -I$(PROTOBUF_PATH_INCLUDE)
TARGETOBJ            = task.o task-linux.o 

.PHONY: all clean cleanall

all: $(TARGETOBJ)

%.o: %.cpp *.hpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ *.pb.*
