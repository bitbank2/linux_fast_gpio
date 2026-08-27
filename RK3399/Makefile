CFLAGS=-c -std=c99 -Wall -O2
LINKFLAGS=-lm

all: fast_gpio

fast_gpio: main.o
	$(CC) main.o $(LINKFLAGS) -o fast_gpio

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm -rf *.o fast_gpio

