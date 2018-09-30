PROJECT := QDHTTP

CC ?= gcc
CFLAGS := -Wall -Werror -pedantic -ggdb -Iinclude -std=c11 -fdiagnostics-color=always
LFLAGS :=

ifndef NO_COLOR
CFLAGS += -fdiagnostics-color=always
endif

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -ggdb
endif

SRC := src/
OBJ := objects/
BIN := bin/

TARGET := $(BIN)qdhttp

SRCDIRS := src src/* src/*/* src/*/*/*
SOURCES := $(foreach DIR, $(SRCDIRS), $(wildcard $(DIR)/*.c))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(SOURCES))

BEG =	echo -e -n "  \033[32m$(1)$(2)...\033[0m" ; echo -n > /tmp/.`whoami`-build-errors
END =	if [[ -s /tmp/.`whoami`-build-errors ]] ; then \
		echo -e -n "\r\033[1;33m$(1)$(2)\033[0m\n"; \
		cat /tmp/.`whoami`-build-errors; \
	else \
		echo -e -n "\r  \033[1;32m$(1)$(2)\033[0m\033[K\n"; \
	fi

INFO = echo -e -n "\r  \033[1;34m$(1)$(2)\033[0m\n"

ERRORFUNC = echo -e " \033[1;31m! Fatal error encountered.\033[0m"; \
	cat /tmp/.`whoami`-build-errors; \
	exit 1;

ERRORS = 2>>/tmp/.`whoami`-build-errors || { $(call ERRORFUNC) }

ERRORSS = >>/tmp/.`whoami`-build-errors || { $(call ERRORFUNC) }

BEGRM = echo -e -n "  \033[31m$(1)$(2)...\033[0m" && echo -n > /tmp/.`whoami`-build-errors
ENDRM = echo -e -n "\r  \033[1;31m$(1)$(2)\033[0m\033[K\n"

.PHONY: all clean init

# Prevents Make from removing intermediary files on failure
.SECONDARY:
# Disable built-in rules
.SUFFIXES:

all: init $(TARGET)
	@$(call INFO, "--", "Building $(PROJECT)...Done!");

init:
	@$(call INFO, "--", "Building $(PROJECT)...");

run: # Won't recompile
	@$(TARGET) -C ../web

$(OBJ)%.o: %.c
	@$(call BEG, "CC", "$<")
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS) $(ERRORS)
	@$(call END, "CC", "$<")

$(TARGET): $(OBJECTS)
	@$(call BEG, "LD", "$<")
	@mkdir -p $(dir $@)
	@$(CC) $(OBJECTS) -o $(TARGET) $(LFLAGS) $(ERRORS)
	@$(call END, "LD", "$<")

clean:
	@$(call BEGRM, "RM", "$(BIN) $(OBJ)")
	@$(RM) -rf $(BIN) $(OBJ)
	@$(call ENDRM, "RM", "$(BIN) $(OBJ)")
