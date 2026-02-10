SRC := src
OBJ := obj
BIN := bin
EXECUTABLE:= shell

SRCS := $(wildcard $(SRC)/*.c)
OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))
INCS := -Iinclude/
DIRS := $(OBJ)/ $(BIN)/
EXEC := $(BIN)/$(EXECUTABLE)

CC := gcc
CFLAGS := -g -Wall -std=c99 $(INCS) -D_POSIX_C_SOURCE=200809L
LDFLAGS :=

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC) $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c $(wildcard include/*.h)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(EXEC)
	$(EXEC)

clean:
	rm -f $(OBJ)/*.o $(EXEC)

$(shell mkdir -p $(DIRS))

.PHONY: run clean all
