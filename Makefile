SUBDIRS = apps/alarm apps/alarm/alarm_module1 apps/tun-driver apps/tests

all:
	@echo "This Makefile is only used to clean up the sources"

subdirs:
	@for d in $(SUBDIRS); do \
		cd $$d; \
		$(MAKE) clean; \
		cd -;\
	done

clean: subdirs
