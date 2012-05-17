XX       = g++
CFLAGS   = -Wall -ggdb -O3 -D_DEBUG
INCLUDES = -I. -I/usr/local/include
LIB	     = -lboost_thread -lboost_system -L/usr/local/lib -Wl,-Bstatic -ljson -lhiredis -Wl,-Bdynamic -lmysqlclient -lpthread -lscws 
SOURCES  = $(wildcard *.cpp)
OBJS     = $(patsubst %.cpp,%.o,$(SOURCES))
SUBOBJS  = ./key-process-engine/*.o ./utility/*.o ./server-skeleton/*.o ./search-kernel/*.o
TARGET   = ../bin/search_engine

all: $(TARGET)

$(TARGET): $(OBJS) $(SUBOBJS)
	$(XX) $(OBJS) $(SUBOBJS) -o $(TARGET) $(LIB)

%.o: %.cpp
	$(XX) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SUBOBJS):
	@make_all_sub_dir()\
	{\
		for dir in `ls`; do\
			if [ -d $$dir ]; then\
				cd $$dir;\
				make_all_sub_dir;\
				if [ -f makefile -o -f Makefile ]; then\
					make;\
				fi;\
				cd ..;\
			fi;\
		done;\
	};\
	make_all_sub_dir

clean:
	rm -rf *.o $(TARGET) $(VERSION)
	@make_clean_sub_dir()\
	{\
		for dir in `ls`; do \
			if [ -d $$dir ]; then \
				cd $$dir;\
				make_clean_sub_dir;\
				if [ -f makefile -o -f Makefile ]; then\
					make clean;\
				fi;\
				cd ..;\
			fi;\
		done;\
	};\
	make_clean_sub_dir

.PHONY: rebuild clean all c r

rebuild: clean all

c: clean

r: rebuild
