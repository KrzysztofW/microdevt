#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2017, Krzysztof Witek
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# The full GNU General Public License is included in this distribution in
# the file called "LICENSE".
#

SUBDIRS = $(shell find apps/* -type d| grep -v kicad)
SUBDIRS+= crypto docs

all:
	@echo -n "This Makefile is only used to check compilation of all "
	@echo "modules, perform basics tests and clean up the sources."

check:
	@for d in $(SUBDIRS); do \
		if [ $$d = "docs" ]; then continue; fi; \
		cd $$d; \
		$(MAKE) clean; \
		if [ $$d = "apps/tests" ] || [ $$d = "crypto" ]; then \
			$(MAKE) check || exit 1; \
		else \
			$(MAKE) || exit 1; \
		fi; \
		cd -; \
	done
	$(MAKE) clean

subdirs:
	@for d in $(SUBDIRS); do \
		cd $$d; \
		$(MAKE) clean; \
		cd -;\
	done
	@rm -rf docs/xml

clean: subdirs
