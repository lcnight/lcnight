CC 			= gcc
XX 			= g++
LIBS 		= -L.
INCLUDES 	= -I.
CFLAG 		= -Wall -ggdb
CPPFLAG 	= -Wall -ggdb
CSRCS 		= $(wildcard *.c)
CPPSRCS 		= $(wildcard *.cpp)
COBJS      	= $(patsubst %.c,%.o, $(CSRC))
CPPOBJS    	= $(patsubst %.cpp,%.o, $(CPPSRC))

all:tags $(CSRCS) $(CPPSRCS)
	$(XX) $(CSRCS) $(CPPSRCS) $(CPPFLAG) $(LIBS) -o main

debug:tags $(CSRCS) $(CPPSRCS)
	$(XX) $(CSRCS) $(CPPSRCS) $(CPPFLAG) $(LIBS) -DDEBUG -o main

%.o:%.c
	$(CC) $(CFLAG) $(INCLUDES) $(LIBS) -c $< -o $@

%.o:%.cpp
	$(XX) $(CPPFLAG) $(INCLUDES) $(LIBS) -c $< -o $@


.PHONY: tags

tags:
	ctags -R *
