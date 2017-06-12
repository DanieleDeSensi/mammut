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

