# Thomas Daley
# September 13, 2021

# A generic build template for C/C++ programs

# executable name
EXE = bastos

# C compiler
CC = gcc
# linker
LD = gcc

# Terminal info
TERM = VT100

# C flags
CFLAGS = -O0 -g
# C++ flags
CXXFLAGS =
# Preprocessor flags
CPPFLAGS = -D$(TERM)=1 -Wall -Werror -I ..
# dependency-generation flags
DEPFLAGS = -MMD -MP
# linker flags
LDFLAGS =
# library flags
LDLIBS = -lm

# build directories
BIN = bin
OBJ = obj
SRC = ..

SOURCES := $(wildcard $(SRC)/*.c ./*.c)

OBJECTS := \
	$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(wildcard $(SRC)/*.c)) \
	$(patsubst ./%.c, $(OBJ)/%.o, $(wildcard ./*.c))

# include compiler-generated dependency rules
DEPENDS := $(OBJECTS:.o=.d)

# compile C source
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@
# link objects
LINK.o = $(LD) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

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

# memcheck the program
.PHONY: memcheck
memcheck: $(BIN)/$(EXE)
	valgrind --leak-check=full --show-leak-kinds=all $(BIN)/$(EXE)

# test_bugs: separate binary (bio.c + test_bugs.c, no bastos.c)
TEST_BUGS_EXE = $(BIN)/test_bugs
TEST_BUGS_OBJECTS = $(OBJ)/bio.o $(OBJ)/test_bugs.o

$(TEST_BUGS_EXE): $(OBJ) $(BIN) $(TEST_BUGS_OBJECTS)
	$(LD) $(LDFLAGS) $(TEST_BUGS_OBJECTS) $(LDLIBS) -o $@

.PHONY: test_bugs
test_bugs: $(TEST_BUGS_EXE)
	./$(TEST_BUGS_EXE)

# remove previous build and objects
.PHONY: clean
clean:
	$(RM) -rf $(BIN)
	$(RM) -rf $(OBJ)

# remove everything except source
.PHONY: reset
reset:
	$(RM) -r $(OBJ)
	$(RM) -r $(BIN)

-include $(DEPENDS)
