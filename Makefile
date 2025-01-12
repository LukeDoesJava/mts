all: mts

mts: mts.o
	gcc -Wall -g -o mts mts.c thread_logic.c -lpthread 

clean:
	rm -f mts.o mts


