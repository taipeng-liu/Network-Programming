#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include "message.c"
#include "slidingWindow.c"

#define MAX_PACKET_SIZE 1472
#define HEADER_LEN 19
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-HEADER_LEN
#define MIN(a,b) a>b?b:a
#define MAX(a,b) a>b?a:b
#define DEBUG 1
#define MAX_INT 2147483647
#define TIMEOUT_US 500000

struct myMsg *tmmsg;
char tbytes[MAX_PACKET_SIZE];
char tcontent[MAX_CONTENT_SIZE];
int sws;
int threshold;

void incSws(void) {
	if (sws <= threshold)
		sws *= 2;
	else if (sws < MAX_SWS)
		sws++;
}

void read_bytes_to_sw(FILE *fp, unsigned long long int *left, int seq, int *offset, int idx) {
	int toread = MIN(*left, MAX_CONTENT_SIZE);
	size_t nread;
	char isLast = 'n';

	memset(tcontent, 0 ,MAX_CONTENT_SIZE);
	memset(tmmsg, 0, sizeof(struct myMsg));
	memset(tbytes, 0, MAX_PACKET_SIZE);
	
	nread = fread(tcontent, sizeof(char), toread, fp);
	if (nread < 1) {
		//Read nothing
		fprintf(stderr, "fread(): read nothing\n");
		return;
	}

	*left -= nread;
	if (*left <= 0) {
		isLast = 'y';
	}

	formMyMsg(tmmsg, 1, 404, seq, *offset, isLast, nread, tcontent);

	myMsgToBytes(tbytes, tmmsg);

	//store in sw
	memcpy(sw[idx], tbytes, MAX_PACKET_SIZE);
	ackd[idx] = -1; //read but not sent

	*offset = (*offset + 1)%MAX_INT;
}

void sendFile(int mySocket, struct sockaddr *raddr, 
		FILE *fp, unsigned long long int left)
{
	socklen_t len = sizeof(*raddr);
	struct sockaddr_in src;

	int seq = 0, offset = 0, dup = 0;

	int i, idx, oldLAF, LFS;

	threshold = MAX_SWS/2;
	sws = MIN_SWS;

	while (left > 0) { //TODO other conditions
		oldLAF = LAF;
		dup = 0;

		//read the file
		for (i = 0; i < sws && left > 0; i++) {
			idx = (LAF+1+i)%MAX_SWS;

			if (ackd[idx] == 1) {
				seq = (LAF+1+i)%MAX_SEQ_NO;
				read_bytes_to_sw(fp, &left, seq, &offset, idx);
			}
		}

		#if DEBUG
		printf("Send: SWS=%d\n", sws);
		#endif

		//send sw
		for (i = 0; i <sws; i++) {
			idx = (LAF+1+i)%MAX_SWS;
			if (ackd[idx] != 1) {
				sendto(mySocket, sw[idx], MAX_PACKET_SIZE, 0, raddr, len);
				ackd[idx] = 0;
				
				#if DEBUG
				memset(tmmsg, 0, sizeof(struct myMsg));
				bytesToMyMsg(tmmsg, sw[idx]);
				printf("Send: seq=%d, offset=%d\n", 
						(LAF+1+i)%MAX_SEQ_NO, tmmsg->offset);
				#endif
			} else {
				break;
			}
		}

		LFS = (LAF + i)%MAX_SEQ_NO;
		#if DEBUG
		printf("Send: seq [%d, %d]\n", LAF+1, LFS);
		#endif
		
		//recvfrom
		do {
			memset(tmmsg, 0, sizeof(struct myMsg));
			memset(tbytes, 0, MAX_PACKET_SIZE);
			if (recvfrom(mySocket, tbytes, MAX_PACKET_SIZE, 0, 
						(struct sockaddr*)&src, &len) < 0) 
			{	//TIMEOUT
				threshold = sws/2;
				sws = MIN_SWS;

				#if DEBUG
				printf("TIMEOUT, oldLAF=%d, LAF=%d, LFS=%d\n", oldLAF, LAF, LFS);
				#endif
				break;
			}

			bytesToMyMsg(tmmsg, tbytes);
			
			if (tmmsg->type == 0) { /* an ACK */
				if (tmmsg->seq < LAF) {
					tmmsg->seq += MAX_SEQ_NO;
				}
				if (tmmsg->seq - LAF - 1 < sws) {
					LAF = tmmsg->seq%MAX_SEQ_NO;
				}
				#if DEBUG
				printf("ReceiveACK: %d\n", tmmsg->seq);
				#endif
			}
		} while(LAF != LFS);

		if (LAF == LFS) {
			//All packet transferred
			incSws();
		}
		
		//set ackd[idx] to 1 from oldLAF to LAF
		for (i = 0; i< (LAF+MAX_SEQ_NO-oldLAF)%MAX_SEQ_NO; i++) {
			ackd[(i+oldLAF+1)%MAX_SWS] = 1;
		}
	}

	fclose(fp);
}

void reliablyTransfer(char *rhostname, unsigned short int rport,
		char *filepath, unsigned long long int nbytes)
{
	int mySocket;
	FILE *fp = fopen(filepath, "r");
	struct sockaddr_in raddr;
	time_t start, end;
	size_t fsize;
	double dif;
	unsigned long long int left;
	struct timeval tv;
	
	//Init
	init_sw();
	tmmsg = malloc(sizeof(struct myMsg));

	//Check if file exists
	if (fp == NULL) {
		perror("fopen");
		return;
	}

	//get file size (bytes)
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	left = MIN(fsize, nbytes);

	//create my socket
	if ( (mySocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		exit(1);
	}
	
	//Setup timeout
	tv.tv_sec = 0;
	tv.tv_usec = TIMEOUT_US;
	if (setsockopt(mySocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0) {
		fprintf(stderr, "setsockopt():error\n");
		return;
	}
		

	//set receiver's address
	memset(&raddr, 0, sizeof(raddr));
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(rport);
	raddr.sin_addr.s_addr = inet_addr(rhostname);

	//main logic
	time(&start);
	sendFile(mySocket, (struct sockaddr*)&raddr, fp, left);
	time(&end);
	dif = difftime(end, start);

	printf("Send: file=%s, size=%llu, time=%.2lfs\n", filepath, left, dif);
	close(mySocket);
}
