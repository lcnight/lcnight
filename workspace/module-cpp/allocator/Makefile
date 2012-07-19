CC = gcc
XX = g++
SRC = $(wildcard *.cpp)
CFLAG = -Wall -ggdb

all:
	$(XX) $(CFLAG) -o main $(SRC)

debug:
	$(XX) $(CFLAG) -DDEBUG -o main $(SRC)
