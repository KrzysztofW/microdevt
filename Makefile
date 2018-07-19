SUBDIRS = $(shell find apps/* -type d)
SUBDIRS+= crypto

all:
	@echo "This Makefile is only used to clean up the sources"

subdirs:
	@for d in $(SUBDIRS); do \
		cd $$d; \
		$(MAKE) clean; \
		cd -;\
	done

clean: subdirs
