CC=/usr/bin/gcc
CC_OPTS=-g3
CC_LIBS=-pthread
CC_DEFINES=
CC_INCLUDES=
CC_ARGS=${CC_OPTS} ${CC_LIBS} ${CC_DEFINES} ${CC_INCLUDES}

all: clean reliable_sender reliable_receiver
.PHONY=clean

reliable_sender: sender.c
	@${CC} ${CC_ARGS} -o reliable_sender sender.c

reliable_receiver: receiver.c
	@${CC} ${CC_ARGS} -o reliable_receiver receiver.c

clean:
	@rm -f reliable_* *.o *.mytest

