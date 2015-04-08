export MAMMUTROOT           = $(realpath ..)
export INCS                 = -I$(MAMMUTROOT)
export LDFLAGS              = -L$(MAMMUTROOT)/mammut 
export LDLIBS               = -lm -pthread -lrt -lmammut -lprotobuf-lite

.PHONY: all clean cleanall

all:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE; \
	done
clean:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE clean; \
	done 
cleanall:
	for MODULE in $(MODULES) ; do \
		$(MAKE) -C $$MODULE cleanall; \
	done 

