SRCS=hwcomp.c
PROG=hwcomp
CC=clang
VERSION='"0.1"'
all:
	${CC} ${SRCS} -DVERSION=${VERSION} -o ${PROG} 

debug: 
	${CC} ${SRCS} -no-pie -DVERSION=${VERSION} -g -o ${PROG}
