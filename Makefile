SHELL=bash

.PHONY: all clean webserver run test

all: webserver

webserver:
	@$(MAKE) -C webserver all

run:
	@$(MAKE) -C webserver all run #Compile and run

test: clean webserver
	@cd unittest; ./insecure.sh ../webserver/src  ../webserver/include
# TODO: Reenable these
#	@$(MAKE) -C webserver run& thePID=$!
#	@cd unittest; $(RM) -rf results/run1; ./check.sh run1
#	@kill $thePID

clean:
	@$(MAKE) -C webserver clean
	@$(RM) -rf unittest/results/*
