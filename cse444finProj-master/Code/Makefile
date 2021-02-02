SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,${SRC})
EXE = a.out
COMPILE = gcc
FLAG = -Werror -Wall -Wextra -Wpedantic -Wshadow -O2 -g
ASMFLAG = -Werror -Wall -Wextra -Wpedantic -O0 -g -S
SUFFIX =  -lpcap -pthread

DIR_OBJ = object
DIR_UTIL = util
DIR_UT = unit_tests
MAKE_O = make

DIR_OBJ_SRC = $(wildcard ${DIR_OBJ}/*.c)
DIR_OBJ_OBJ = $(patsubst %.c,%.o,${DIR_OBJ_SRC})

DIR_UTIL_SRC = $(wildcard ${DIR_UTIL}/*.c)
DIR_UTIL_OBJ = $(patsubst %.c,%.o,${DIR_UTIL_SRC})

DIR_UT_SRC = $(wildcard ${DIR_UT}/*.c)
DIR_UT_OBJ = $(patsubst %.c,%.o,${DIR_UT_SRC})

all : ${EXE}

${EXE} : ${OBJ} ${DIR_OBJ_OBJ} ${DIR_UTIL_OBJ} ${DIR_UT_OBJ}
	${COMPILE} ${FLAG} ${OBJ} ${DIR_OBJ_OBJ} ${DIR_UTIL_OBJ} ${DIR_UT_OBJ} -o $@ ${SUFFIX}

%.o : %.c
	${COMPILE} ${FLAG} -c $< -o $@

asmdb : $(patsubst %o,%s,${OBJ}) ${EXE}

%.s : %.c
	${COMPILE} ${ASMFLAG} $<

clean :
	rm -rf ${OBJ}
	rm -rf $(patsubst %.o,%.s,${OBJ})
	rm -rf ${EXE}
	cd ${DIR_OBJ}; make clean
	cd ${DIR_UTIL}; make clean
	cd ${DIR_UT}; make clean

run : all
	./${EXE}

runasm : asmdb
	./${EXE}

valgrind : ${EXE}
	make clean all
	valgrind --leak-check=full -s ./${EXE}
