.POSIX:
.SUFFIXES:

# makefile for c yaml parser example
# author: smavros

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2

all: my_parser.x

my_parser.x : my_parser.c 
	$(CC) $(CFLAGS) $< -lyaml -o $@

clean :
	rm *.x

.PHONY : clean
