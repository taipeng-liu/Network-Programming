#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "handle.c"

#define MAX_NODE 256
#define BUF_SIZE 100
#define PACKET_SIZE 1100

/* GLOBAL PARAMETERS FROM main.c */
extern int globalMyID;
extern int globalSocketUDP;
extern struct sockaddr_in globalNodeAddrs[MAX_NODE];

void hackyBroadcast(const char* msg, int len) {
	int i;
	for(i = 0; i < MAX_NODE; i++) {
		if(i != globalMyID) sendto(globalSocketUDP, msg, len, 0,
			(struct sockaddr*)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
	}
}

void* announceToNeighbors(void* unusedParam) {
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 300 * 1000 * 1000; //300 ms
	while(1)
	{
		hackyBroadcast("HEREIAM", 7);
		nanosleep(&sleepFor, 0);
	}
}

/* TIMER FUNCTION
 * Call checkHeartbeat() every 1s */
void* checkCurNeighbors(void* unusedParam) {
	while (1)
	{
		updateNeighbor();
		sleep(1);
	}
}

void *lspSendTimer(void *unused) {
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 400 * 1000 * 1000; //400 ms
	while (1){
		broadcastMyState();
		nanosleep(&sleepFor, 0);
	}
}

short int getSenderID(char *fromAddr) {
	short int senderID = -1;
	if(strstr(fromAddr, "10.1.1.")) {
		senderID = atoi(strchr(strchr(strchr(fromAddr,'.')+1,'.')+1,'.')+1);
	}
	return senderID;
}

void listenForNeighbors() {
	char fromAddr[BUF_SIZE];
	unsigned char recvPacket[PACKET_SIZE];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	int bytesRecvd;

	while(1) {
		//receive packet
		theirAddrLen = sizeof(theirAddr);
		memset(recvPacket, 0, PACKET_SIZE);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvPacket, PACKET_SIZE , 0, 
					(struct sockaddr*)&theirAddr, &theirAddrLen)) == -1) {
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		
		//Save readable address at fromAddr
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, BUF_SIZE);

		//Resolve senderID and add to neighbor
		short int senderID = getSenderID(fromAddr);
		if (senderID != -1) {
			if (!isNeighbor(senderID)) {
				addNewNeighbor(senderID);
			}
			updateHeartbeat(senderID);
		}

		//Execute instructions
		if(!strncmp(recvPacket, "send", 4)) {
			handleManagerPacket('s', recvPacket);
		} else if(!strncmp(recvPacket, "cost", 4)) {
			handleManagerPacket('c', recvPacket);
		} else if(!strncmp(recvPacket, "TNET", 4)) {
			handleMyPacket((int)senderID, recvPacket);
		}
	}

	close(globalSocketUDP);
}
