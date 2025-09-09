# FIXME: Install cppcheck, https://github.com/danmar/cppcheck
CC=gcc
CCOPTS=-Wall -Wextra -Wconversion
CPPCHECK=cppcheck
CPPCHECKOPTS=--addon=misra --check-level=normal --enable=all --inconclusive --safety --checkers-report=report.txt

all: examples

examples: examples.c
	$(CC) $(CCOPTS) examples.c -o examples

check:
	$(CPPCHECK) $(CPPCHECKOPTS) examples.c

clean:
	rm -f examples