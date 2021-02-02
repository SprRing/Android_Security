SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,${SRC})
COMPILE = gcc
FLAG = -Werror -Wall -Wextra -Wpedantic -Wshadow -O2 -g
ASMFLAG = -Werror -Wall -Wextra -Wpedantic -O0 -g -S
SUFFIX =  -lpcap -pthread

all : ${OBJ}

%.o : %.c
	${COMPILE} ${FLAG} -c $<

asmdb : $(patsubst %o,%s,${OBJ})

%.s : %.c
	${COMPILE} ${ASMFLAG} $<

clean :
	rm -rf ${OBJ}
	rm -rf $(patsubst %.o,%.s,${OBJ})