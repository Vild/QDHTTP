PROJECT := QDHTTP
SHELL = bash

CC ?= gcc
CFLAGS_BASE := -Iinclude -std=gnu1x -finline-functions -fPIC -fPIE -pipe -D_DEFAULT_SOURCE
LFLAGS := -lm
WARNINGS := -Wall -Wextra -Wshadow
WARNINGS += -Wfloat-equal -Wmissing-prototypes -Wunused-value -Wunused-parameter
WARNINGS += -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes
WARNINGS += -Wsign-compare -Wconversion -Wcast-align -Wparentheses
WARNINGS += -Wsequence-point -Wundef
WARNINGS += -Wpointer-arith -Wnested-externs -Wredundant-decls
WARNINGS += -Wdisabled-optimization

SRC := src/
INC := include/
OBJ := objects/
BIN := bin/


# == Options ==
CFLAGS :=

ifndef NO_COLOR
CFLAGS += -fdiagnostics-color=always
endif

ifdef RELEASE
CFLAGS += -O2 -DNDEBUG
LFLAGS += -Wl,-O1
else
CFLAGS += -ggdb3 -Og
endif

ifndef NO_ARCH_TUNE
CFLAGS += -mtune=native -march=native
endif

ifndef NO_LTO
CFLAGS += -flto
LFLAGS += -flto
endif

ifndef NO_WERROR
WARNINGS += -Werror
endif

# == Add the options to the flags ==

CFLAGS += $(CFLAGS_BASE) $(WARNINGS)
LFLAGS += $(CFLAGS)

# == Directories ==

TARGET := $(BIN)qdhttp

SOURCES := $(shell find $(SRC) -iname "*.c" -not -path '*/\.*')
HEADERS := $(shell find $(INC) -iname "*.h" -not -path '*/\.*')
OBJECTS = $(patsubst $(SRC)%.c, $(OBJ)%.o, $(SOURCES))

# == Fun color output ==
OFF = \033[1;0m
BOLD =\033[1;1m
BLUE = $(BOLD)\033[1;34m
GREEN = $(BOLD)\033[1;32m
RED = $(BOLD)\033[1;31m
YELLOW = $(BOLD)\033[1;33m

BEG =	echo -e -n "$(1)$(2)$(OFF) \033[1m$(3)...$(OFF)" ; echo -n > /tmp/.`whoami`-build-errors
END =	if [[ -s /tmp/.`whoami`-build-errors ]] ; then \
		if cut -d':' -f4 /tmp/.`whoami`-build-errors | grep -q error; then \
			echo -e -n "\r$(RED)$(2)$(OFF) \033[1m$(3)   $(OFF)\n"; \
			cat /tmp/.`whoami`-build-errors; \
			rm /tmp/.`whoami`-build-errors || true; \
			exit -1; \
		else \
			echo -e -n "\r$(YELLOW)$(2)$(OFF) \033[1m$(3)   $(OFF)\n"; \
			cat /tmp/.`whoami`-build-errors; \
			rm /tmp/.`whoami`-build-errors || true; \
		fi \
	else \
		echo -e -n "\r$(1)$(2)$(OFF) \033[1m$(3)$(OFF)\033[K\n"; \
	fi

INFO = echo -e -n "$(GREEN)$(1) $(2)$(OFF)\n"

ERRORS = 2>>/tmp/.`whoami`-build-errors || true
ERRORSS = >>/tmp/.`whoami`-build-errors || true

# == Build rules ==

.PHONY: all clean init depend

# Prevents Make from removing intermediary files on failure
.SECONDARY:
# Disable built-in rules
.SUFFIXES:

all: init $(TARGET)
	@$(call INFO,"::","Building $(PROJECT)... Done!");

init:
	@$(call INFO,"::","Building $(PROJECT)...");

$(OBJ)%.o: $(SRC)%.c
	@$(call BEG,$(BLUE),"  -\> CC","$@ \<-- $<")
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS) $(ERRORS)
	@$(call END,$(BLUE),"  -\> CC","$@ \<-- $<")

$(TARGET): $(OBJECTS)
	@$(call BEG,$(BLUE),"  -\> LD","$@ \<-- $^")
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) -o $(TARGET) $(LFLAGS) $(ERRORS)
	@$(call END,$(BLUE),"  -\> LD","$@ \<-- $^")

clean:
	@$(call INFO,"::","Removing generated files...");
	@$(call BEG,$(BLUE),"  -\> RM",".depend")
	@$(RM) -rf .depend
	@$(call END,$(BLUE),"  -\> RM",".depend")
	@$(call BEG,$(BLUE),"  -\> RM","$(BIN)")
	@$(RM) -rf $(BIN)
	@$(call END,$(BLUE),"  -\> RM","$(BIN)")
	@$(call BEG,$(BLUE),"  -\> RM","$(OBJ)")
	@$(RM) -rf $(OBJ)
	@$(call END,$(BLUE),"  -\> RM","$(OBJ)")

includePaths = $(shell echo | gcc -E -Wp,-v - 2>&1 | grep "\#include <" -A32 | egrep "^ " | sed "s/ /\-I/" | tr "\n" " ")
.depend: $(SOURCES) $(HEADERS)
	@$(call INFO,"::","Generating dependencies...");
	@$(call BEG,$(BLUE),"  -\> RM","$(BIN)")
	@$(RM) -rf ./.depend
	@$(call END,$(BLUE),"  -\> RM","$(BIN)")
	@$(call BEG,$(BLUE),"  -\> makedepend","$@ \<-- $(SOURCES)")
	@makedepend $(includePaths) -p$(OBJ) -- $(CFLAGS_BASE) -- $(SOURCES) -f- | sed -e "s/src\///" -e "s/\.\///" > $@ $(ERRORS)
	@$(call END,$(BLUE),"  -\> makedepend","$@ \<-- $(SOURCES)")
include .depend
