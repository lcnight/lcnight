CC         = gcc
XX         = g++
CFLAGS     = -Wall -fPIC -ggdb
CXXFLAGS   = -Wall -fPIC -ggdb -Wno-non-virtual-dtor
LIBS       =  #`mysql_config --libs` -ldl -lmemcached
INCLUDES   = -I./
CSOURCES   = $(wildcard *.c)
CXXSOURCES = $(wildcard *.cpp */*.cpp)
COBJS      = $(patsubst %.c,%.o, $(CSOURCES))
CXXOBJS    = $(patsubst %.cpp,%.o, $(CXXSOURCES))

BASENAME   = test
LINKNAME   = lib$(BASENAME).so
SONAME	   = lib$(BASENAME).so.1
TARGET     = lib$(BASENAME).so.1.0.0

EXENAME    = test

BUILD_DIR  = build/


all:
	$(XX) -ggdb -o $(EXENAME) $(CXXSOURCES) $(LIBS)

%.o: %.cpp
	$(XX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	-rm -rfv *.o */*.o $(TARGET)
	-rm -rfv $(LINKNAME) $(SONAME) $(TARGET) 
	
c: clean

r: clean all
	
tags:
	-ctags -R

.PHONY : all c r cmd test

cmd:
	@echo CXXSRC: $(CXXSOURCES)
#	ar -rs libstat.a $(COBJS) $(CXXOBJS)
#	/sbin/ldconfig -n .
