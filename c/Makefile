# Makefile for slit

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -O0 -fsanitize=address,undefined

TARGET = slit
SRCS = main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all build clean test valgrind

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c slit.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

test: build
	@echo "Tests not yet implemented"

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version
