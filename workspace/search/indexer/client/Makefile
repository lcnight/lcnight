##############################################
# make file for indexer
############################################## 
CC		    = gcc
XX		    = g++
LIBS	    = -lhiredis
INCLUDES    = -I../src
#-Wno-non-virtual-dtor -Wno-reorder
XX_FLAGS    = -Wall -ggdb
CC_SRC	    = $(wildcard *.c */*.c)
XX_SRC	    = $(wildcard *.cpp */*.cpp)
CC_OBJS	    = $(patsubst %.c,%.o, $(CC_SRC))
XX_OBJS	    = $(patsubst %.cpp,%.o, $(XX_SRC))
BIN		    = idx-cli

all:tags $(BIN)

$(BIN): $(CC_OBJS) $(XX_OBJS)
	$(XX) $(XX_FLAGS) $(INCLUDES) $(LIBS) $(CC_OBJS) $(XX_OBJS) -o $@

test:
	#$(XX) $(XX_FLAGS) $(INCLUDES) $(LIBS) $(CC_OBJS) $(XX_OBJS) -o $@

%.o: %.cpp
	$(XX) $(XX_FLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(XX) $(XX_FLAGS) $(INCLUDES) -c $< -o $@

.PHONY : tags clean c r cmd

clean:
	-@rm -vf *.o *.so */*.o */*.so $(BIN)

c:clean

r: clean all

tags:
	-@ctags -R *;

cmd:
	@echo -e "$(BIN)\n$(CC_SRC), $(XX_SRC)\n$(CC_OBJS), $(XX_OBJS)"
