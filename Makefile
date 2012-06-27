all: baseline

baseline: 
	gcc -g -Wall -o baseline baseline.c

clean:
	rm -rf baseline
