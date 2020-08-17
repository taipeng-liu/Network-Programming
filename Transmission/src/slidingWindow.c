#include <stdio.h>

#define MAX_RWS 1024
#define MAX_SWS 1024
#define MAX_SEQ_NO 2048

#define MIN_SWS 16

#define MAX_PACKET_SIZE 1472
#define HEADER_LEN 19
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-HEADER_LEN

char rw[MAX_RWS][MAX_CONTENT_SIZE];
int recvd[MAX_RWS]; //Initialize to 0, value is equal to the length of content
int NFE;

char sw[MAX_SWS][MAX_PACKET_SIZE];
int ackd[MAX_SWS]; //Initialize to 1
int LAF = -1;

//NOTE: seq, NFE, and LAF range from 0 to MAX_SEQ_NO-1

/* -----------------------------receive window-----------------------------*/
void init_rw(void) {
	int i;

	for (i = 0; i < MAX_RWS; i++) {
		memset(rw[i], 0, MAX_CONTENT_SIZE);
		recvd[i] = 0;
	}

	NFE = 0;
}

//Whenever receive a packet, call this function
int handle_rw(char *content, int clen, int seq, FILE *fp) {
	int idx;
	int i;

	if (seq < NFE)
		seq += MAX_SEQ_NO;

	if (seq - NFE < MAX_RWS) {
		idx = (seq % MAX_RWS);

		if (recvd[idx] == 0) {//Not duplicate
			recvd[idx] = clen;
			memcpy(rw[idx], content, clen); //store the content
		}

		//Deliver frames to app
		for (i = 0; i < MAX_RWS; i++) {
			idx = (i + NFE) % MAX_RWS;

			if (recvd[idx] == 0) break;

			//write to file
			fwrite(rw[idx], sizeof(char), recvd[idx], fp);

			recvd[idx] = 0;
		}

		NFE = (NFE + i)%MAX_SEQ_NO;
	}
	
	return (NFE-1+MAX_SEQ_NO)%MAX_SEQ_NO;
}


/* -----------------------------send window-------------------------------*/

void init_sw(void) {
	int i;
	
	for (i = 0; i <MAX_SWS; i++) {
		memset(sw[i], 0, MAX_PACKET_SIZE);
		ackd[i] = 1;
	}
}
