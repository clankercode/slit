# Makefile for slit

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -O0 -fsanitize=address,undefined

TARGET = slit
TEST_TARGETS = test_buffer test_terminal
SRCS = main.c buffer.c terminal.c
OBJS = $(SRCS:.c=.o)

TEST_BUFFER_SRCS = test/test_buffer.c buffer.c
TEST_BUFFER_OBJS = $(TEST_BUFFER_SRCS:.c=.o)

TEST_TERMINAL_SRCS = test/test_terminal.c terminal.c
TEST_TERMINAL_OBJS = $(TEST_TERMINAL_SRCS:.c=.o)

.PHONY: all build clean test test_buffer test_terminal valgrind

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c slit.h terminal.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TEST_TARGETS) $(OBJS) $(TEST_BUFFER_OBJS) $(TEST_TERMINAL_OBJS)

test_buffer: $(TEST_BUFFER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_terminal: $(TEST_TERMINAL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test: test_buffer test_terminal
	./test_buffer
	./test_terminal

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version
