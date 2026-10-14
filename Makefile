CC = gcc

CFLAGS = -Wall -Wextra -g -O2 -Iinclude

SRC = $(wildcard src/*.c)

TARGET = server

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) -pthread

clean:
	rm -f $(TARGET)