#!bin/bash
# this is the Makefile for Project3

CC = gcc
CFLAGS = -I. -g
TARGETS = runsim testsim
#RUNSIM = runsim.c license.c detachandremove.c
#TESTSIM = testsim.c license.c
SRC = runsim.c detachandremove.c license.c
OBJ = runsim.o detachandremove.o license.o
SRC2 = testsim.c
OBJ2 = testsim.o

all: $(TARGETS)

$(OBJ): $(SRC)
	$(CC) -c $(SRC)

$(OBJ2): $(SRC2)
	$(CC) -c $(SRC2)

runsim: $(OBJ)
	$(CC) -o $@ $^

testsim: $(OBJ2)
	$(CC) -o $@ $^ license.o

clean:
	/bin/rm '-f' *.o $(TARGETS) *.log