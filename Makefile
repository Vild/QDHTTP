SHELL=bash

.PHONY: all clean webserver run test

all: webserver

webserver:
	@$(MAKE) -C webserver all

test: clean webserver
	@cd unittest; ./insecure.sh ../webserver/src ../webserver/include
	@./test.sh

clean:
	@$(MAKE) -C webserver clean
	@$(RM) -rf unittest/results/*
