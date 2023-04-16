CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -lpthread

.PHONY: clean

proj2: proj2.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) -f proj2.zip *.o proj2

pack:
	zip proj2.zip *.c *.h Makefile