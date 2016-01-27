# makefile for c yaml parser example
# author: smavros

CC = gcc
CFLAGS = -Wall -pedantic -std=c99

parser : 
	$(CC) $(CFLAGS) my_parser.c -lyaml -o parser

.PHONY : clean
clean :
	rm parser
