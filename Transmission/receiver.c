#include <stdio.h>
#include <stdlib.h>

#include "src/reliable_receiver.c"

int main(int argc, char **argv) {
	unsigned short int port;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <rcv_port> <file_path_to_write>\n\n", argv[0]);
		exit(1);
	}
	
	port = (unsigned short int)atoi(argv[1]);
	
	reliablyReceive(port, argv[2]);

	return 0;
}
