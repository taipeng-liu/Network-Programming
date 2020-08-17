CC=/usr/bin/gcc
CC_OPTS=-g3
CC_LIBS=
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

all : http_client http_server
.PHONY=clean

http_client: http_client.c
	@${CC} ${CC_ARGS} -o http_client http_client.c

http_server: http_server.c
	@${CC} ${CC_ARGS} -o http_server http_server.c

clean:
	@rm -f http_client http_server *.o output
