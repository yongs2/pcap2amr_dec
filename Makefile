# ----------------------------------------------------------------------- #
# Description : pcap2amr_dec makefile                                     #
#                                                                         #
# This code is pcap2amr_dec                                               #
# ----------------------------------------------------------------------- #

SHELL   = /bin/sh
AR      = ar
TARGET  = .
CC		= gcc

BINARY_NAME		= pcap2amr_dec
OBJECT_PREFIX	= ${HOME}/.objs
OBJECT_PATH		= ${OBJECT_PREFIX}/func/utils/${BINARY_NAME}
EXEC_1			= ${OBJECT_PATH}/${BINARY_NAME}
EXEC_2			= ${OBJECT_PATH}/pcm2amr_enc
EXEC_3			= ${OBJECT_PATH}/amr2pcm_dec

OPENCORE_PATH = /usr/local

INCDIR	= 	-I${HOME}/inc -I./ -I${OPENCORE_PATH}/include/opencore-amrnb -I${OPENCORE_PATH}/include/opencore-amrwb

LDIR    = 	-L${HOME}/lib -L${OPENCORE_PATH}/lib

CFLAGS	= ${G_PROJ_DEFINE} ${SYSTEM_DEFINE} ${SYSTEM_CFLAGS} -D__GLOBAL__ -Wall -D_REENTRANT -g 

LFLAGS	= -lopencore-amrnb -lopencore-amrwb

SRCS_1	= pcap2amr_dec.c amrnb.c amrwb.c
OBJS_1	= ${SRCS_1:%.c=${OBJECT_PATH}/%.o}

SRCS_2	= pcm2amr_enc.c amrnb.c amrwb.c
OBJS_2	= ${SRCS_2:%.c=${OBJECT_PATH}/%.o}

SRCS_3	= amr2pcm_dec.c amrnb.c amrwb.c
OBJS_3	= ${SRCS_3:%.c=${OBJECT_PATH}/%.o}

all : dir ${EXEC_1} ${EXEC_2} ${EXEC_3}

dir:
	mkdir -p ${OBJECT_PATH}

${EXEC_1}: ${OBJS_1}
	${CC} -o ${EXEC_1} ${OBJS_1} ${LDIR} ${CFLAGS} ${LFLAGS}

${EXEC_2}: ${OBJS_2}
	${CC} -o ${EXEC_2} ${OBJS_2} ${LDIR} ${CFLAGS} ${LFLAGS}

${EXEC_3}: ${OBJS_3}
	${CC} -o ${EXEC_3} ${OBJS_3} ${LDIR} ${CFLAGS} ${LFLAGS}

${OBJECT_PATH}/%.o:%.c
	${CC} -c $< ${CFLAGS} ${INCDIR} -o $@

install: ${EXEC_1}
	install ${EXEC_1} ${TARGET}
	install ${EXEC_2} ${TARGET}
	install ${EXEC_3} ${TARGET}

clean:
	rm -f ${OBJS_1} ${EXEC_1} ${OBJS_2} ${EXEC_2} ${OBJS_3} ${EXEC_3} core
