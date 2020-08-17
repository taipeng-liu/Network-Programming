#include <stdio.h>
#include <stdlib.h>

#include "src/reliable_sender.c"

int main(int argc, char **argv) {
	unsigned short int rport;
	unsigned long long int nbytes;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s <rcv_hostname> <rcv_port> <file_path_to_read> <bytes_to_send>\n", argv[0]);
		exit(1);
	}
	
	rport = (unsigned short int)atoi(argv[2]);
	nbytes = atoll(argv[4]);

	reliablyTransfer(argv[1], rport, argv[3], nbytes);

	return 0;
}
