SRC:=$(wildcard *.cpp) $(wildcard *.c)
HEADER:=$(wildcard *.h)
OBJ:=$(SRC:.c=.o)
OBJ:=$(OBJ:.cpp=.o)
INCLUDEDIR=.
CC=clang++
LINK=clang++
MAKEARGS=-Wdeprecated-writable-strings
CFLAGS=-g $(MAKEARGS) -Winvalid-source-encoding # -Werror 
OPTIMIZE=-O0
BIN=tga2pif
LFLAGS=-v

INCLUDES=-I. -I/user/local/include -I/opt/local/include

all:depend bin after_process
	#cp tga2pif ../../../work/tool
	ls

depend:
	-echo $(OBJ)
	-rm -f tmpdepend
	for i in $(SRC); do $(CC) $(INCLUDES) -MM $$i >>tmpdepend; done
	sed -e "s!^[^ ]!$(OBJDIR)/&!" <tmpdepend >.Dependencies
	#sed <tmpdepend >.Dependencies
	#-rm -f tmpdepend

-include .Dependencies
%.o:%.c 
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTIMIZE) -o $@ -c $<
%.o:%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTIMIZE) -o $@ -c $<

bin:$(OBJ)
	$(LINK) $(LFLAGS) $(OBJ) -o $(BIN)

after_process:
	-rm -rf $(OBJ)

clean:
	-rm -f .Dependencies
	-rm -rf $(OBJ)

