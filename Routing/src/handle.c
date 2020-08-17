#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "update.c"
#include "logger.c"
#include "message.c"
#include "Dijkstra.c"

#define MAX_NODE 256
#define BUF_SIZE 100
#define PACKET_SIZE 1100
#define MAX_TTL 256

/* GLOBAL PARAMETERS FROM main.c */
extern int globalMyID;
extern int globalSocketUDP;
extern struct sockaddr_in globalNodeAddrs[MAX_NODE];

/* HANDLE MANAGER'S INSTRUCTION
 * flag: 's' - send instruction; 'c' - cost instruction */
void handleManagerPacket(char flag, char *recvPacket) {
	short int dest;
	char msg[BUF_SIZE];
	int i, newCost, nextHop;

	memcpy(&dest, recvPacket + 4, sizeof(short int));
	dest = ntohs(dest);
			
	switch (flag) {
		case 's':
			for (i = 0; i < BUF_SIZE; i++) {
				msg[i] = *(recvPacket+4+sizeof(short int)+i);
				if (msg[i] == '\0') 
					break;
			}
			initPacketTo(dest, msg);
			break;
		case 'c':
			memcpy(&newCost, recvPacket+4+sizeof(short int), sizeof(int));
			newCost = ntohl(newCost);
			updateCost((int)dest, newCost);
			break;
		default:
			return;
	}
}

//handle a broadcast of state
void handleType1(int senderID, struct myPacket *mpp) {
	int state[MAX_NODE];
	
	//Transfer content to state
	memset(state, 0, MAX_NODE*sizeof(int));
	unzipState(state, mpp->content, mpp->clen);

	//DEBUG print out state
	//for (int i = 0; i < MAX_NODE; i++) {
	//	if (state[i] != 0) printf("%d : %d\n", i, state[i]);
	//}

	updateGlobalStateByRow(mpp->src, state);
	
	broadcastPacket(mpp, senderID);
}

//handle an e2e message
void handleType2(int senderID, struct myPacket *mpp) {
	int nextHop;
	if ((int)(mpp->dest) == globalMyID) {
		logreceive(mpp->content, mpp->clen);
		sendBackAck(senderID, mpp->src, mpp->seq);
	} else {
		nextHop = getNextHop(mpp->dest);
		if (nextHop > -1) {
			forwardPacket(mpp, nextHop);
		} else {
			//DEBUG
			logunreachable(mpp->dest);
			sendBackUnreachable(senderID, mpp->src, mpp->seq);
		}
	}
}

//handle an ACK
void handleType0(int senderID, struct myPacket *mpp) {
	int src_seq;
	memcpy(&src_seq, mpp->content, sizeof(int));
	src_seq = ntohl(src_seq);
	
	if (findPending(mpp->dest, src_seq)) {
		recordAck(mpp->dest, src_seq);
	}
	
	if ((int)(mpp->dest) == globalMyID) {
		//DEBUG
		//logack(mpp->src);
	} else {
		sendBackAck(getNextHop(mpp->dest), mpp->dest, src_seq);
	}
}

//handle an unreachable
void handleTypeN1(int senderID, struct myPacket *mpp) {
	//TODO
	return;
}

void handleMyPacket(int senderID, char *recvPacket) {
	int i;
	int nextHop;
	struct myPacket *mpp = malloc(sizeof(struct myPacket));

	msgToPacket(mpp, recvPacket);

	if (mpp->seq > getSequence(mpp->src)) {
		//DEBUG
		//printf("Recv from %d: type = %hd, src = %hd, dest = %hd, seq = %d, ttl = %d, clen = %d\n", senderID, mpp->type, mpp->src, mpp->dest, mpp->seq, mpp->ttl, mpp->clen);
		switch (mpp->type) {
			case 1: //Type 1 : broadcast of the state of the src
				handleType1(senderID, mpp);
				break;
			case 2: //Type 2: end-to-end message
				handleType2(senderID, mpp);
				break;
			case 0: //ACK
				handleType0(senderID, mpp);
				break;
			case -1: //Unreachable
				handleTypeN1(senderID, mpp);
				break;
			default:
				return;
		}
	updateSequence(mpp->src, mpp->seq);
	}
}
