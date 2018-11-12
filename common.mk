dirty = $(shell git diff-files --quiet \
	&& git diff-index --cached --quiet HEAD -- || echo "-dirty")

REVISION = $(shell git rev-parse --short HEAD)$(dirty)
SOURCES += version.c

revision:
	echo "const char *revision = \"$(REVISION)\";\n" > version.c
	echo "extern const char *revision;" > version.h

clean_common:
	@rm -f version.[ch]
