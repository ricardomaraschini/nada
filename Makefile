INSTALLPATH=/opt/nada
CC=gcc
LIBS=-lm -lmysqlclient
CFLAGS=-g -L/usr/lib64/mysql -L/usr/lib/mysql -Wall
MACROS=-D"INSTALLPATH=\"$(INSTALLPATH)\""

all: baseline

baseline: 
	$(CC) $(CFLAGS) $(LIBS) $(MACROS) -o nada dblayer.c baseline.c iniparser/iniparser.c iniparser/dictionary.c

clean:
	rm -rf nada 

install:
	mkdir -p $(INSTALLPATH) 
	cp -Rfp nada $(INSTALLPATH)
	cp -Rfp baseline.ini $(INSTALLPATH) 

uninstall:
	rm -rf $(INSTALLPATH)
