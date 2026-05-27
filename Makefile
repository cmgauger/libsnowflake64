################################################################################
#                                                                              #
# Copyright (c) 2026 Christian Gauger-Cosgrove                                 #
#                                                                              #
# Permission is hereby granted, free of charge, to any person obtaining a copy #
# of this software and associated documentation files (the "Software"), to     #
# deal in the Software without restriction, including without limitation the   #
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  #
# sell copies of the Software, and to permit persons to whom the Software is   #
# furnished to do so, subject to the following conditions:                     #
#                                                                              #
# The above copyright notice and this permission notice (including the next    #
# paragraph) shall be included in all copies or substantial portions of the    #
# Software.                                                                    #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS #
# IN THE SOFTWARE.                                                             #
#                                                                              #
################################################################################



# Project-wide settings
debug ?= 0
NAME := libsnowflake64
BUILD_DIR := build
INCLUDE_DIR := include
LIB_DIR := lib
SRC_DIR := src
TEST_SRC_DIR := tests
BIN_DIR := bin

LIBLOGGER_DIR   = extern/liblogger
LIBWINDEP_DIR   = extern/windows_dependencies
LIBARGTABLE_DIR = extern/libargtable3

# These are for MacOS
SYS_TREE_INCLUDE = $(LIBWINDEP_DIR)/include/sys
CUNIT_LDFLAGS = $(shell /usr/local/bin/pkg-config cunit --libs)
$(BIN_DIR)/test : CPPFLAGS += $(shell /usr/local/bin/pkg-config cunit --cflags-only-I)

CPPFLAGS += -I$(LIBLOGGER_DIR)/include -I$(SYS_TREE_INCLUDE) -I$(LIBARGTABLE_DIR)/include

vpath %.c $(SRC_DIR) $(TEST_SRC_DIR) $(LIBLOGGER_DIR)/src $(LIBARGTABLE_DIR)/src

# object file paths
OBJS := $(addprefix $(BUILD_DIR)/, $(patsubst %.c, %.o, \
	$(notdir $(wildcard $(SRC_DIR)/*.c))))

TEST_OBJS := $(addprefix $(BUILD_DIR)/, $(patsubst %.c, %.o, \
	$(notdir $(wildcard $(TEST_SRC_DIR)/*.c $(LIBLOGGER_DIR)/src/*.c $(LIBARGTABLE_DIR)/src/*.c))))

# Compilers & Utilities
CC := gcc
AR := ar
RM := rm -rf

# General flag settings
#   CFLAGS
#     -std=c89: use ISO C89 standard
#     -fPIC: compile as position-independent code
#   CPPFLAGS
#     -D_LIB
#   ARFLAGS
#     rcs: create the library and its symbol table, all in one go
CFLAGS += -std=c89 -fPIC
CPPFLAGS += -I$(INCLUDE_DIR)/ -D_LIB
ARFLAGS += rcs

# Debug & Release build specific settings
#     Debug Builds
#     ============
#   CFLAGS
#     -g: include debugging symbols
#     -O0: no optimizations
#   CPPFLAGS
#     -D_DEBUG
#
#     Release Builds
#     ==============
#   CFLAGS
#     -Ofast: optimize for speed
ifeq ($(debug), 1)
	CFLAGS += -g -O0
	CPPFLAGS += -D_DEBUG
else
	CFLAGS += -Ofast
endif

.PHONY: all clean test

all: $(NAME)

clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(LIB_DIR)

$(LIB_DIR) : ; mkdir -p $(LIB_DIR)

$(BUILD_DIR) : ; mkdir -p $(BUILD_DIR)

$(BIN_DIR) : ; mkdir -p $(BIN_DIR)

$(NAME): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $(addprefix $(LIB_DIR)/, $(addsuffix .a, $@)) $^

$(BUILD_DIR)/%.o : %.c | $(BUILD_DIR)
	$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

## Tests

$(BIN_DIR)/test : $(TEST_OBJS) $(OBJS) | $(BIN_DIR)
	$(CC) -o $@ $^ $(CUNIT_CPPFLAGS) $(CUNIT_LDFLAGS)

test : $(BIN_DIR)/test
	$(BIN_DIR)/test batch

