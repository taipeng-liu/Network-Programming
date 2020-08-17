#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include "message.c"
#include "slidingWindow.c"

#define MAX_PACKET_SIZE 1472
#define HEADER_LEN 19
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-HEADER_LEN
#define DEBUG 1

struct myMsg *tmmsg;
char tcontent[MAX_CONTENT_SIZE];
char tbytes[MAX_PACKET_SIZE];

void send_ack(int mySocket, int fileNo, int seq, struct sockaddr_in saddr, socklen_t len) {
	int n;
	memset(tmmsg, 0, sizeof(struct myMsg));
	memset(tbytes, 0, MAX_PACKET_SIZE);

	formMyMsg(tmmsg, 0, fileNo, seq, 0, 'y', 0, "");
	n = myMsgToBytes(tbytes, tmmsg);
	sendto(mySocket, tbytes, n, 0, (struct sockaddr*)&saddr, len);
	#if DEBUG
	printf("SendACK: seq=%d\n",seq);
	#endif
}

void receiveFile(int mySocket, FILE *fp)
{
	struct sockaddr_in saddr;
	socklen_t len = sizeof(saddr);

	int last_recv_seq;
	int last_seq = -1;
	
	while(1) {
		memset(tbytes, 0 ,MAX_PACKET_SIZE);
		memset(tmmsg, 0, sizeof(struct myMsg));

		//write new packet to tbytes
		recvfrom(mySocket, tbytes, MAX_PACKET_SIZE, 0, (struct sockaddr*)&saddr, &len);

		//write new packet to tmmsg
		if (bytesToMyMsg(tmmsg, tbytes)  == 0) {
			fprintf(stderr, "receive no byte\n");
		}
		
		//the last packet, mark
		if (tmmsg->isLast == 'y') {
			last_seq = tmmsg->seq;
		}
		
		#if DEBUG
		printf("Receive: seq=%d, offset=%d, isLast=%c\n",
			tmmsg->seq, tmmsg->offset, tmmsg->isLast);
		#endif

		//sliding window
		last_recv_seq = handle_rw(tmmsg->content, tmmsg->clen, tmmsg->seq, fp);
	
		send_ack(mySocket, tmmsg->fileNo, last_recv_seq, saddr, len);
		
		if (last_recv_seq == last_seq) {
			return;
		}
	}
}


void reliablyReceive(unsigned short int port, char *filepath) {
	int mySocket;
	struct sockaddr_in bindAddr;

	FILE *fp;

	//Init
	init_rw();
	tmmsg = malloc(sizeof(struct myMsg));

	//socket()
	if ( (mySocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

	//bind
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(mySocket, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) < 0)
	{
		perror("bind");
		close(mySocket);
		exit(1);
	}
	
	//Create file
	fp = fopen(filepath, "w");
	if (fp == NULL) {
		fprintf(stderr, "fopen(): error\n");
		return;
	}

	//main logic
	receiveFile(mySocket, fp);

	//return
	close(mySocket);
	fclose(fp);
	free(tmmsg);
}
