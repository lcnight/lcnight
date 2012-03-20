CC = gcc
XX = g++
SRC = $(wildcard *.cpp)
CFLAG = -Wall -ggdb

all: tags
	$(XX) $(CFLAG) -o main $(SRC)

debug: tags
	$(XX) $(CFLAG) -DDEBUG -o main $(SRC)

clean:
	rm -rvf *.o *.a *.so *~

tags:
	ctags -R * 

.PHONY: clean c r tags t

c: clean

r: clean all

t: tags
