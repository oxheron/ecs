TARGET_EXEC = test 
CC = clang++

SRC = test.cpp 
OBJ = $(SRC:.cpp=.o)
BIN = bin

INC_DIR_SRC = -Isrc 
INC_DIR_LIB = -I../ 

DEBUGFLAGS = $(INC_DIR_SRC) $(INC_DIR_LIB) -Wall -g
RELEASEFLAGS = $(INC_DIR_SRC) $(INC_DIR_LIB) -O2
ASMFLAGS = $(INC_DIR_SRC) $(INC_DIR_LIBS) -Wall
LDFLAGS = $(LIBS) -lm -fuse-ld=mold

.PHONY: all libs clean test

all: 
	$(MAKE) -j8 bld
	$(MAKE) link

dirs:
	mkdir -p ./$(BIN)

link: $(OBJ)
	$(CC) -o $(BIN)/$(TARGET_EXEC) $^ $(LDFLAGS)

bld: 
	$(MAKE) clean
	$(MAKE) dirs
	$(MAKE) obj

obj: $(OBJ)

%.o: %.cpp
	$(CC) -std=c++20 -o $@ -c $< $(DEBUGFLAGS)

build: dirs link

run:
	./$(BIN)/$(TARGET_EXEC)

clean:
	clear
	rm -rf $(BIN) $(OBJ)
