
CFLAGS=-I . -I .. -O2 --std=c99
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
