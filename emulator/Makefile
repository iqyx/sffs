#CDEBUGFLAGS=-fprofile-arcs -ftest-coverage -pg -g
CFLAGS=-I . -I .. -O3 --std=c99  $(CDEBUGFLAGS)
LDFLAGS=-O3  $(CDEBUGFLAGS)
CC=gcc
LD=gcc


all: test1

sffs:
	$(CC) $(CFLAGS) -c ../sffs.c

flash_emulator:
	$(CC) $(CFLAGS) -c flash_emulator.c

test1: flash_emulator sffs
	$(CC) $(CFLAGS) -c sffs_test1.c
	$(LD) $(LDFLAGS) sffs.o sffs_test1.o flash_emulator.o -o sffs_test1
