CC=gcc
CPPFLAGS=-D_DEFAULT_SOURCE
CFLAGS=-std=c99 -pedantic -Werror -Wall -Wextra -Wvla -fvisibility=hidden -fPIC
LDFLAGS=-shared -Wl,--no-undefined
CFILES=src/alignment/alignment.c src/malloc.c src/wrappers.c
OFILES=$(CFILES:.c=.o)

library: $(OFILES)
	$(CC) $(LDFLAGS) -o libmalloc.so $^

check: library tests/test_lib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_lib.c -o test_lib -L. -l:libmalloc.so
	./tests/test.sh
clean:
	$(RM) libmalloc.so test_lib $(OFILES)
