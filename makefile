CC = gcc
CFLAGS = -g -pedantic -std=c99 -Wall -Werror -Wextra -I. -pthread -O2

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

TARGET = nyuenc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
