CC=/usr/bin/gcc
CC_OPTS=-g3
CC_LIBS=-pthread
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

all : clean ls_router
.PHONY=clean

ls_router: main.c
	@${CC} ${CC_ARGS} -o ls_router main.c

clean:
	@rm -f ls_router *.o output mytest/log/*.log
