GIT_VERSION := "$(shell git describe --abbrev=4 --dirty --always --tags)"

clean_common:
	@echo ""
