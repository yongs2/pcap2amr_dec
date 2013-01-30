# ----------------------------------------------------------------------- #
# Description : pcap2amr_dec makefile                                     #
#                                                                         #
# This code is pcap2amr_dec                                               #
# ----------------------------------------------------------------------- #

SHELL   = /bin/sh
AR      = ar
TARGET  = .
CC		= gcc

BINARY_NAME   = pcap2amr_dec
OBJECT_PREFIX = ${HOME}/.objs
OBJECT_PATH   = ${OBJECT_PREFIX}/func/utils/${BINARY_NAME}
EXEC          = ${OBJECT_PATH}/${BINARY_NAME}

OPENCORE_PATH = /usr/local

INCDIR	= 	-I${HOME}/inc -I./ -I${OPENCORE_PATH}/include/opencore-amrnb -I${OPENCORE_PATH}/include/opencore-amrwb

LDIR    = 	-L${HOME}/lib -L${OPENCORE_PATH}/lib

CFLAGS	= ${G_PROJ_DEFINE} ${SYSTEM_DEFINE} ${SYSTEM_CFLAGS} -D__GLOBAL__ -Wall -D_REENTRANT -g 

LFLAGS	= -lopencore-amrnb -lopencore-amrwb

SRCS    = pcap2amr_dec.c amrnb.c amrwb.c

OBJS    = ${SRCS:%.c=${OBJECT_PATH}/%.o}

all : dir ${EXEC}

dir:
	mkdir -p ${OBJECT_PATH}

${EXEC}: ${OBJS}
	${CC} -o ${EXEC} ${OBJS} ${LDIR} ${CFLAGS} ${LFLAGS}

${OBJECT_PATH}/%.o:%.c
	${CC} -c $< ${CFLAGS} ${INCDIR} -o $@

install: ${EXEC}
	install ${EXEC} ${TARGET}

clean:
	rm -f ${OBJS} ${EXEC} core
