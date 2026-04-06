CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -O0 -fsanitize=address,undefined

TARGET = slit
SRCS = main.c buffer.c terminal.c render.c layout.c spinner.c status.c tee.c debug.c completion.c
OBJS = $(SRCS:.c=.o)

TEST_TARGETS = test_buffer test_terminal test_render
TEST_BUFFER_SRCS = test/test_buffer.c buffer.c
TEST_BUFFER_OBJS = $(TEST_BUFFER_SRCS:.c=.o)

TEST_TERMINAL_SRCS = test/test_terminal.c terminal.c
TEST_TERMINAL_OBJS = $(TEST_TERMINAL_SRCS:.c=.o)

TEST_RENDER_SRCS = test/test_render.c render.c
TEST_RENDER_OBJS = $(TEST_RENDER_SRCS:.c=.o)

.PREFIX = /usr/local

.PHONY: all build clean test test_buffer test_terminal test_render valgrind install

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c slit.h terminal.h buffer.h render.h layout.h spinner.h status.h tee.h debug.h completion.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TEST_TARGETS) $(OBJS) $(TEST_BUFFER_OBJS) $(TEST_TERMINAL_OBJS) $(TEST_RENDER_OBJS)

test_buffer: $(TEST_BUFFER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_terminal: $(TEST_TERMINAL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_render: $(TEST_RENDER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test: test_buffer test_terminal test_render
	./test_buffer
	./test_terminal
	./test_render

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version

install: build
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
