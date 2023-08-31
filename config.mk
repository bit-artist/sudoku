VERSION = 0.1

BIN_NAME_SOLVER = sudoku-solver
MAN_NAME_SOLVER = $(BIN_NAME_SOLVER).6
BIN_NAME_EDITOR = sudoku-editor
MAN_NAME_EDITOR = $(BIN_NAME_EDITOR).6
INSTALL_PATH = /usr/local/bin
MANPAGE_PATH = /usr/local/share/man/man6

# Default working directory of the editor
WORKDIR = .local/share/$(BIN_NAME_EDITOR)

# For generating the manpage
TITLE_SOLVER = SUDOKU-SOLVER
TITLE_EDITOR = SUDOKU-EDITOR

CC = gcc
# debug
#CFLAGS = -ggdb -O0 -Wall -Wextra -Wpedantic
CFLAGS = -O2
CFLAGS_SOLVER = $(CFLAGS)
CFLAGS_EDITOR = $(CFLAGS)
LDFLAGS =

