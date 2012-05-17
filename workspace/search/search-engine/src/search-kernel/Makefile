XX	= g++
CFLAGS	= -Wall -O3 -ggdb -D_DEBUG
INCLUDE	= -I. -I../
SOURCES	= $(wildcard *.cpp)
OBJS	= $(patsubst %.cpp, %.o, $(SOURCES))

all: $(OBJS)

%.o: %.cpp
	$(XX) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf *.o

rebuild: clean all

c: clean

r: rebuild


