all: baseline

baseline: 
	gcc -g -L/usr/lib64/mysql -Wall -lmysqlclient -o baseline dblayer.c baseline.c

clean:
	rm -rf baseline
