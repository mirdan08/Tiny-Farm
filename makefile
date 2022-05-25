CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

MAIN=farm

all:$(MAIN)

$(MAIN):$(MAIN).o xerrori.o


.PHONY:clean zip

# target che cancella eseguibili e file oggetto
clean:
	rm -f $(MAIN) *.o  

# target che crea l'archivio dei sorgenti
zip:
	zip $(MAIN).zip makefile *.c *.h *.py *.md
