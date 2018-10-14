SHELL=bash

.PHONY: all clean webserver run test

all: webserver

webserver:
	@$(MAKE) -C webserver all

test: clean
	@$(MAKE) -C webserver all NO_COLOR=1 NO_WERROR=1 RELEASE=1
	@cd unittest; ./insecure.sh ../webserver/src ../webserver/include
	@./test.sh

clean:
	@$(MAKE) -C webserver clean
	@$(RM) -rf unittest/results/*
