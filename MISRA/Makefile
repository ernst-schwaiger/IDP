# FIXME: Install cppcheck, https://github.com/danmar/cppcheck
CC=gcc
CCOPTS=-Wall -Wextra -Wconversion
all: examples

examples: examples.c
	$(CC) $(CCOPTS) examples.c -o examples

clean:
	rm -f examples