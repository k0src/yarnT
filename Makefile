CC = gcc
SRC_DIR = src
BIN_DIR = bin

all: $(BIN_DIR)/yarnT

$(BIN_DIR)/yarnT: $(SRC_DIR)/yarnT.c
	$(CC) $< -o $@ -Wall -Wextra -pedantic -std=c99

clean:
	-rm -rf $(BIN_DIR)/*.out
	-rm -rf $(BIN_DIR)/*.o