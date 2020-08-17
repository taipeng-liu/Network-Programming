/* This is message.c */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_NODE 256
#define PACKET_SIZE  1100
#define MAX_CONTENT_SIZE 1000
#define MAX_TTL 256
#define MAX_WAITING_TIME 2

/* GLOBAL PARAMETERS FROM main.c */
extern int globalMyID;
extern int globalSocketUDP;
extern struct sockaddr_in globalNodeAddrs[MAX_NODE];

/* GLOBAL VARIABLES FROM main.c */
extern int routingTable[MAX_NODE];
extern int globalState[MAX_NODE][MAX_NODE];
extern int globalCostTable[MAX_NODE];
extern int sequenceTable[MAX_NODE];

/* FUNCTION FROM update.c */
extern int getNextHop(int dest);
extern int unzipState(int *state_i, char *state_c, int stateLen);
extern int scanNodeState(int src, char *state_c);
extern int getSequence(int dest);
extern void updateSequence(int dest, int newSeq);

/* Instruction of myPakcet
 *Msg format: TNET(4) + type(2) + src(2) + dest(2) + seq (4) + ttl(4) + clen(4) + content(...)
 *
 *Message type: 1 - broadcast of the state of src node (no ACK needed)
 *		2 - manager's massage (ACK needed)
 *		0 - ACK
 *	       -1 - Unreachable (no ACK required)
 *
 * Basic steps to send a packet:
 * - createMyPacket(mpp, ...);
 * - packetToMsg(msg_buf, mpp);
 * - sendTo(..., msg_buf);
 *
 * Basic steps to receive myPacket:
 * - receive(..., msg_buf, ...);
 * - msgToPacket(mpp, msg_buf);
 */

//Data structure
struct myPacket {
	//To be send in a message
	short int type; //2 bytes
	short int src;  //2 bytes
	short int dest; //2 bytes
	int seq;	//4 bytes
	int ttl;	//4 bytes
	int clen;	//4 bytes
	char content[MAX_CONTENT_SIZE];  //extra content

	struct myPacket *next;
};

//Pending list keep all pending packets
struct myPacket *pendingList = NULL;

//////////////////////////////////PENDING FUNCTIONS/////////////////////////////////
struct myPacket *findPending(int src, int seq) {
	struct myPacket *found = pendingList;

	while (found) {
		if (found->src == src && found->seq == seq)
			break;
		found = found->next;
	}

	return found;
}

int addPending(struct myPacket *mpp) {
	if (pendingList) {
		mpp->next = pendingList;
	}
	pendingList = mpp;
	return 0;
}

//key = (src, seq)
int rmPending(int src, int seq) {
	struct myPacket *cur = pendingList, *prev = pendingList;

	while (cur) {
		if (cur->src == src && cur->seq == seq) {
			if (cur == pendingList) {
				pendingList = cur->next;
			} else {
				prev->next = cur->next;
			}

			cur->next = NULL;
			free(cur);
			return 0;
		}

		prev = cur;
		cur = cur->next;
	}

	return -1;
}

void recordAck(int src, int seq) {
	struct myPacket *found = findPending(src, seq);

	if (found) {
		found->clen = -1;
	}
}

void *checkPendingTimer(void *data) {
	sleep(MAX_WAITING_TIME);

	struct myPacket *mpp = (struct myPacket *)data;
	if (mpp) {
		if (mpp->clen != -1)
			logunreachable(mpp->dest);
		else 
			rmPending(mpp->src, mpp->seq);
	}
}

void goCheckPendingTimer(struct myPacket *mpp) {
	pthread_t checker;
	pthread_create(&checker, 0, checkPendingTimer, (void*)mpp);
}

//////////////////////////////////Format Functions//////////////////////////////////
int createMyPacket(struct myPacket *mpp, short int type, short int src, short int dest, int seq, int ttl, int clen, char *content) {
	mpp->type = type;
	mpp->src  = src;
	mpp->dest = dest;
	mpp->seq  = seq;
	mpp->ttl  = ttl;
	mpp->clen = clen;
	memcpy(mpp->content, content, clen);

	mpp->next = NULL;

	return 4+3*sizeof(short int)+3*sizeof(int)+clen;
}

int packetToMsg(char *msg, struct myPacket *mpp) {
	short int net_type = htons(mpp->type);
	short int net_src = htons(mpp->src);
	short int net_dest = htons(mpp->dest);
	int net_seq = htonl(mpp->seq);
	int net_ttl = htonl(mpp->ttl);
	int net_clen = htonl(mpp->clen);

	strcpy(msg,"TNET");
	memcpy(msg+4,                                   &net_type,    sizeof(short int));
	memcpy(msg+4+sizeof(short int),                 &net_src,     sizeof(short int));
	memcpy(msg+4+2*sizeof(short int),               &net_dest,    sizeof(short int));
	memcpy(msg+4+3*sizeof(short int),               &net_seq,     sizeof(int));
	memcpy(msg+4+3*sizeof(short int)+sizeof(int),   &net_ttl,     sizeof(int));
	memcpy(msg+4+3*sizeof(short int)+2*sizeof(int), &net_clen,    sizeof(int));
	memcpy(msg+4+3*sizeof(short int)+3*sizeof(int), mpp->content, mpp->clen);

	return 4+3*sizeof(short int)+3*sizeof(int)+mpp->clen;
}

int msgToPacket(struct myPacket *mpp, char *msg) {
	int i;

	if (strncmp(msg, "TNET", 4)) 
		return -1;
	
	memcpy(&(mpp->type),   msg+4,                                   sizeof(short int));
	mpp->type = ntohs(mpp->type);
	memcpy(&(mpp->src),    msg+4+sizeof(short int),                 sizeof(short int));
	mpp->src  = ntohs(mpp->src);
	memcpy(&(mpp->dest),   msg+4+2*sizeof(short int),               sizeof(short int));
	mpp->dest = ntohs(mpp->dest);
	memcpy(&(mpp->seq),    msg+4+3*sizeof(short int),               sizeof(int));
	mpp->seq  = ntohl(mpp->seq);
	memcpy(&(mpp->ttl),    msg+4+3*sizeof(short int)+sizeof(int),   sizeof(int));
	mpp->ttl  = ntohl(mpp->ttl);
	memcpy(&(mpp->clen),   msg+4+3*sizeof(short int)+2*sizeof(int), sizeof(int));
	mpp->clen = ntohl(mpp->clen);
	
	memcpy(mpp->content,   msg+4+3*sizeof(short int)+3*sizeof(int), mpp->clen);

	return 4+3*sizeof(short int)+3*sizeof(int)+mpp->clen;
}

////////////////////////////Non-API FUNCTIONS/////////////////////////////////////
void sendMPP(struct myPacket *mpp, int nextHop) {
	char msg[PACKET_SIZE];
	int totalLen;

	memset(msg, 0, PACKET_SIZE);
	totalLen = packetToMsg(msg, mpp);

	sendto(globalSocketUDP, msg, totalLen, 0, (struct sockaddr*)&globalNodeAddrs[nextHop], sizeof(globalNodeAddrs[nextHop]));
}

void broadcastMPP(struct myPacket *mpp, int exclude) {
	char msg[PACKET_SIZE];
	int dest, totalLen;
	
	for (dest = 0; dest < MAX_NODE; dest++) {
		if (dest != exclude && isNeighbor(dest)) {
		//if (dest != exclude) {
			memset(msg, 0, PACKET_SIZE);

			mpp->dest = -1; //Destination doesn't matter in broadcast

			totalLen = packetToMsg(msg, mpp);
			
			sendto(globalSocketUDP, msg, totalLen, 0, (struct sockaddr*)&globalNodeAddrs[dest], sizeof(globalNodeAddrs[dest]));
		}
	}
}

void sendBack(short int type, short int prevHop, short int src, int src_seq) {
	char content[4];
	int net_seq;
	struct myPacket *mpp = malloc(sizeof(struct myPacket));

	net_seq = htonl(src_seq);
	memcpy(content, &net_seq, sizeof(int));

	addSequence(globalMyID);
	createMyPacket(mpp, type, globalMyID, src, getSequence(globalMyID), MAX_TTL, 4, content);

	sendMPP(mpp, prevHop);

	free(mpp);
}

////////////////////////////////sendMPP() APIs///////////////////////////////////////

int initPacketTo(int dest, char *content) {
	struct myPacket *mpp = malloc(sizeof(struct myPacket));
	int nextHop = getNextHop((int)dest);

	if (nextHop > -1) {
		addSequence(globalMyID);
		
		createMyPacket(mpp, 2, globalMyID, dest, getSequence(globalMyID), MAX_TTL, strlen(content), content);
		
		sendMPP(mpp, nextHop);

		logsending(dest, nextHop, content, strlen(content));//TODO check
		
		addPending(mpp);

		goCheckPendingTimer(mpp); //TODO update seq when resending

	} else {
		//DEBUG
		logunreachable(dest);
	}

	return 0;
}

void forwardPacket(struct myPacket *mpp, int nextHop) {
	if (--(mpp->ttl)) {
		sendMPP(mpp, nextHop);
		
		addPending(mpp);

		goCheckPendingTimer(mpp);
	}

	logforward(mpp->dest, nextHop, mpp->content, mpp->clen);
}

///////////////////////////////broadcastMPP() APIs////////////////////////////////

void broadcastMyState(void) {
	char myState[MAX_CONTENT_SIZE];
	int stateLen = 0;
	struct myPacket *mpp = malloc(sizeof(struct myPacket));

	memset(myState, 0, MAX_CONTENT_SIZE);
	stateLen = scanNodeState(globalMyID, myState);

	//LAST UPDATE
	addSequence(globalMyID);

	createMyPacket(mpp, 1, globalMyID, -1, getSequence(globalMyID), MAX_TTL, stateLen, myState);

	broadcastMPP(mpp, globalMyID);

	free(mpp);
}

void broadcastPacket(struct myPacket *mpp, int senderID) {
	if (mpp && mpp->ttl) {
		broadcastMPP(mpp, senderID);
	}
}

////////////////////////////////////////sendBack() APIs/////////////////////////

void sendBackAck(short int prevHop, short int src, int src_seq) {
	sendBack(0, prevHop, src, src_seq);
}

void sendBackUnreachable(short int prevHop, short int src, int src_seq) {
	sendBack(-1, prevHop, src, src_seq);
}

