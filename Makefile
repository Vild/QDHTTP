SHELL=bash

.PHONY: all clean webserver run test

all: webserver

webserver:
	@$(MAKE) -C webserver all

run:
	@$(MAKE) -C webserver all run #Compile and run

test: clean webserver
	@cd unittest; ./insecure.sh ../webserver/src ../webserver/include
# TODO: Reenable these
	@#./test.sh

clean:
	@$(MAKE) -C webserver clean
	@$(RM) -rf unittest/results/*
