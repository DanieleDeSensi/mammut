INCS                 = -I../../ -I$(PROTOBUF_PATH_INCLUDE)
TARGETOBJ            = fastflow.o 

.PHONY: clean cleanall

all: $(TARGETOBJ)

%.o: %.cpp *.hpp *.tpp
	$(CXX) $(CXXFLAGS) $(INCS) $(OPTIMIZE_FLAGS) -c -o $@ $<
clean: 
	-rm -fr *.o *~
cleanall:
	-rm -fr *.o *~ *.pb.*
