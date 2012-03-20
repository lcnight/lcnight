CC = gcc
XX = g++
FLAG = -Wall -ggdb 
SOFLAG = -shared -fPIC -Wl,-soname,$(SONAME) 

#specify linked lib and find paths (for header and lib files)
LIB=-lfunc
LIBPATH = -L. -L/usr/local/lib
INCLUDEPATH = -I. -I/usr/local/include

#for version control
MAJOR_VER = 1
MINOR_VER = 0

#for source files of so and exe
SOSRC = func.cpp func.h
SRC = main.cpp

#name for so and exe file
SOLINKNAME = libfunc.so
NAME = main

SONAME = $(SOLINKNAME).$(MAJOR_VER)
SOREALNAME = $(SOLINKNAME).$(MAJOR_VER).$(MINOR_VER)
TARGET = $(SOREALNAME) $(NAME) 

all: $(TARGET) tags

$(SOREALNAME): $(SOSRC)
	$(XX)  $(FLAG) $(SOFLAG) -o $@ $(SOSRC); 
	-@ln -s $(SOREALNAME) $(SOLINKNAME)
	-@ln -s $(SOREALNAME) $(SONAME)

$(NAME):$(SRC)
	$(XX) $(FLAG) -o $@ $(SRC) $(INCLUDEPATH) $(LIBPATH) $(LIB)

clean:
	-@rm -vf $(SONAME) $(SOREALNAME) $(SOREALNAME) $(TARGET)

tags:
	-@ctags -R

r:clean all

.PHONY : all clean r cmd  

cmd:
	@echo "auto variables : $$@", $@,  '  $$^  ', $^, '    $$<   ', $<, "$$?", $?;
	@echo LIBPATTERNS, $(LIBPATTERNSS);
	@echo PATH, $(PATH);
	@echo CC, $(CC);
	@echo "FLAG: ",$(FLAG), $(SOFLAG)
	@echo "NAME: ",$(SONAME), $(SOLINKNAME);
	@echo TARGET, $(TARGET);

func:
	@make_all_sub_dir()\
	{\
	   for dir in `ls`; do \
	    if [ -d $$dir ]; then \
	    echo $$dir;\
	                fi;\
	        done;\
	};\
	make_all_sub_dir
