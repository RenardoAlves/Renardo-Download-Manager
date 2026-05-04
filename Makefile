SHELL = C:/msys64/usr/bin/sh.exe
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -LC:/msys64/mingw64/lib -lcurl

SRC = src/main.c src/downloader.c src/daemon.c
OUT = build/dm.exe

all:
	$(CC) $(SRC) $(CFLAGS) -o $(OUT) $(LDFLAGS)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)