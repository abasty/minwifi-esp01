# Thomas Daley
# September 13, 2021

# A generic build template for C/C++ programs

# executable name
EXE = bastos-linux-amd64

# C compiler
CC = gcc
# linker
LD = gcc

# C flags
CFLAGS = -O0 -g
# C++ flags
CXXFLAGS =
# Preprocessor flags
CPPFLAGS = -Wall -Werror -I ..
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

# Exclude test_bugs.c from the main bastos binary (it has its own main/HAL)
OBJECTS := \
	$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(wildcard $(SRC)/*.c)) \
	$(patsubst ./%.c, $(OBJ)/%.o, $(filter-out ./test_bugs.c, $(wildcard ./*.c)))

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

# Windows cross-compile (amd64), via mingw-w64 (x86_64-w64-mingw32-gcc)
WIN_CC = x86_64-w64-mingw32-gcc
WIN_EXE = bastos-windows-amd64.exe
WIN_OBJ = obj-windows
WIN_LDLIBS = -lm -lws2_32

WIN_OBJECTS := \
	$(patsubst $(SRC)/%.c, $(WIN_OBJ)/%.o, $(wildcard $(SRC)/*.c)) \
	$(patsubst ./%.c, $(WIN_OBJ)/%.o, $(filter-out ./test_bugs.c, $(wildcard ./*.c)))

.PHONY: windows
windows: $(BIN)/$(WIN_EXE)

$(BIN)/$(WIN_EXE): $(SRC) $(WIN_OBJ) $(BIN) $(WIN_OBJECTS)
	$(WIN_CC) $(LDFLAGS) $(WIN_OBJECTS) $(WIN_LDLIBS) -o $@

$(WIN_OBJ):
	mkdir -p $(WIN_OBJ)

$(WIN_OBJ)/%.o:	$(SRC)/%.c
	$(WIN_CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(WIN_OBJ)/%.o:	./%.c
	$(WIN_CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

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
	$(RM) -rf $(WIN_OBJ)

# remove everything except source
.PHONY: reset
reset:
	$(RM) -r $(OBJ)
	$(RM) -r $(WIN_OBJ)
	$(RM) -r $(BIN)

-include $(DEPENDS)
