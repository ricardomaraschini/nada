all: baseline

baseline: 
	gcc -g -L/usr/lib64/mysql -Wall -lm -lmysqlclient -o baseline dblayer.c baseline.c

clean:
	rm -rf baseline
