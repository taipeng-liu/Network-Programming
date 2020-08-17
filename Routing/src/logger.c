#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 100

char buf[2*BUF_SIZE];
char temp[BUF_SIZE];
extern FILE *globalLogFp;

//Log sending
void logsending(int dest, int nexthop, char *msg, int clen) {
	memset(temp, 0, BUF_SIZE);
	memcpy(temp, msg, clen);
	temp[clen] = '\0';
	
	memset(buf, 0, 2*BUF_SIZE);
	sprintf(buf, "sending packet dest %d nexthop %d message %s\n", dest, nexthop, temp);
	fwrite(buf, 1, strlen(buf), globalLogFp);
	fflush(globalLogFp);
}

//Log forward
void logforward(int dest, int nexthop, char *msg, int clen) {
	memset(temp, 0, BUF_SIZE);
	memcpy(temp, msg, clen);
	temp[clen] = '\0';
	
	memset(buf, 0, 2*BUF_SIZE);
	sprintf(buf, "forward packet dest %d nexthop %d message %s\n", dest, nexthop, temp);
	fwrite(buf, 1, strlen(buf), globalLogFp);
	fflush(globalLogFp);
}

//Log receive
void logreceive(char *msg, int clen) {
	memset(temp, 0, BUF_SIZE);
	memcpy(temp, msg, clen);
	temp[clen] = '\0';

	memset(buf, 0, 2*BUF_SIZE);
	sprintf(buf, "receive packet message %s\n", temp);
	fwrite(buf, 1, strlen(buf), globalLogFp);
	fflush(globalLogFp);
}

//Log unreachable
void logunreachable(int dest) {
	memset(buf, 0, 2*BUF_SIZE);
	sprintf(buf, "unreachable dest %d\n", dest);
	fwrite(buf, 1, strlen(buf), globalLogFp);
	fflush(globalLogFp);
}


////////////DEBUG ONLY//////////////////

void logack(int src) {
	memset(buf, 0, 2*BUF_SIZE);
	sprintf(buf, "receive ACK from %d\n", src);
	fwrite(buf, 1, strlen(buf), globalLogFp);
	fflush(globalLogFp);
}

void debugLog(char *msg) {
	fwrite(msg, 1, strlen(msg), globalLogFp);
	fflush(globalLogFp);
}
