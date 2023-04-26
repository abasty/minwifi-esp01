# Thomas Daley
# September 13, 2021

# A generic build template for C/C++ programs

# executable name
EXE = test

# C compiler
CC = gcc
# linker
LD = gcc

# C flags
CFLAGS = -O0 -g
# C++ flags
CXXFLAGS =
# Preprocessor flags
CPPFLAGS = -Wall -I ../..
# dependency-generation flags
DEPFLAGS = -MMD -MP
# linker flags
LDFLAGS =
# library flags
LDLIBS = -lm

# build directories
BIN = bin
OBJ = obj
SRC = ../..

SOURCES := $(wildcard $(SRC)/*.c ./*.c)

OBJECTS := \
	$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(wildcard $(SRC)/*.c)) \
	$(patsubst ./%.c, $(OBJ)/%.o, $(wildcard ./*.c))

# include compiler-generated dependency rules
DEPENDS := $(OBJECTS:.o=.d)

# compile C source
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@
# link objects
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) $(OBJECTS) -o $@

.DEFAULT_GOAL = all

.PHONY: all
all: $(BIN)/$(EXE)

$(BIN)/$(EXE): $(SRC) $(OBJ) $(BIN) $(OBJECTS)
	$(LINK.o)

$(SRC):
	mkdir -p $(SRC)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(OBJ)/%.o:	$(SRC)/%.c
	$(COMPILE.c) $<

$(OBJ)/%.o:	./%.c
	$(COMPILE.c) $<

# force rebuild
.PHONY: remake
remake:	clean $(BIN)/$(EXE)

# execute the program
.PHONY: run
run: $(BIN)/$(EXE)
	./$(BIN)/$(EXE)

# remove previous build and objects
.PHONY: clean
clean:
	$(RM) $(OBJECTS)
	$(RM) $(DEPENDS)
	$(RM) $(BIN)/$(EXE)

# remove everything except source
.PHONY: reset
reset:
	$(RM) -r $(OBJ)
	$(RM) -r $(BIN)

-include $(DEPENDS)
