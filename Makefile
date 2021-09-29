#!bin/bash
# this is the Makefile for Project2

CC = gcc
CFLAGS = -I. -g
TARGETS = runsim testsim
RUNSIM = runsim.c license.c detachandremove.c
TESTSIM = testsim.c
SRC = runsim.c detachandremove.c license.c
OBJ = runsim.o detachandremove.o license.o
OBJ2 = testsim.o

all: $(TARGETS)

$(OBJ): $(SRC)
	$(CC) -c $(SRC)

runsim: $(OBJ)
	$(CC) -o $@ $^

testsim: $(OBJ2)
	$(CC) -o $@ $^

clean:
	/bin/rm '-f' *.o $(TARGETS)