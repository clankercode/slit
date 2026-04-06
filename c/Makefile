# Makefile for slit

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -O0 -fsanitize=address,undefined

TARGET = slit
TEST_TARGET = test_buffer
SRCS = main.c buffer.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = test/test_buffer.c buffer.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

.PHONY: all build clean test valgrind

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c slit.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJS) $(TEST_OBJS)

test_buffer: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test: test_buffer
	./test_buffer

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version
