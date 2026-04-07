CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -pedantic -D_POSIX_C_SOURCE=200809L

TARGET = slit
SRCS = main.c buffer.c terminal.c render.c layout.c spinner.c status.c tee.c debug.c completion.c configfile.c
OBJS = $(SRCS:.c=.o)

TEST_TARGETS = test_buffer test_terminal test_render

PREFIX = /usr/local

.PHONY: all build clean test test_buffer test_terminal test_render valgrind install

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c slit.h terminal.h buffer.h render.h layout.h spinner.h status.h tee.h debug.h completion.h configfile.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TEST_TARGETS) $(OBJS)

test_buffer: test/test_buffer.c buffer.c
	$(CC) $(CFLAGS) -o $@ $^

test_terminal: test/test_terminal.c terminal.c
	$(CC) $(CFLAGS) -o $@ $^

test_render: test/test_render.c render.c
	$(CC) $(CFLAGS) -o $@ $^

test: test_buffer test_terminal test_render
	./test_buffer
	./test_terminal
	./test_render

valgrind: build
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET) --version

install: build
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	./$(TARGET) --generate-man > $(DESTDIR)$(PREFIX)/share/man/man1/$(TARGET).1
