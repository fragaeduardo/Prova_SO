CC     = gcc
CFLAGS = -Wall -Wextra -pthread

pix: pix.c
	$(CC) $(CFLAGS) -o pix pix.c

clean:
	rm -f pix

.PHONY: clean
