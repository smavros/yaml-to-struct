.POSIX:
.SUFFIXES:

# makefile for c yaml parser example
# author: smavros

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2

all: coil_parser.x

coil_parser.x : coil_parser.c 
	$(CC) $(CFLAGS) $< -lyaml -o $@

clean :
	rm *.x

.PHONY : clean
