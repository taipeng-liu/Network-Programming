/* This is update.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_NODE 256
#define BUF_SIZE 100
#define TIME_OUT 1  //1 seconds

/* GLOBAL PARAMETERS FROM main.c */
extern int globalMyID;
extern struct sockaddr_in globalNodeAddrs[MAX_NODE];

/* GLOBAL VARIABLES FROM main.c */
extern int sequenceTable[MAX_NODE];                      //val: seq     init: 0's
extern int routingTable[MAX_NODE];                       //val: nextHop init: -1's
extern int globalState[MAX_NODE][MAX_NODE];              //val: cost    init: 0's
extern int globalCostTable[MAX_NODE];                    //val: cost    init: 1's
extern struct timeval globalLastHeartbeat[MAX_NODE];     //val: hbTime  init: 0's

/* FUNCTION FROM message.c */
extern void broadcastMyState(void);

/* FUNCTION FROM Dijkstra.c */
extern void runDijkstra(void);
extern void getDijkstraResult(int *rt, int len);
extern void killDijkstra(void);

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////READ ONLY OPERATIONS///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

/* ---------SEQUENCE TABLE----------- */
int getSequence(int dest) {
	if (dest > -1 && dest < MAX_NODE) {
		return sequenceTable[dest];
	}
	return -1;
}

/* ----------ROUTING TABLE------------- */
int printRoutingTable(void) {
	int i;

	printf("----Routing Table----\n");
	for (i = 0; i < MAX_NODE; i++) {
		if (routingTable[i] != -1) {
			printf("Dest = %d, NextHop = %d\n", i, routingTable[i]);
		}
	}

	printf("---------------------\n");
}

int getNextHop(int dest) {
	if (dest > -1 && dest < MAX_NODE) {
		return routingTable[dest];
	}
	return -1;
}

/* ----------GLOBAL STATE-------------- */
int getConnState(int src, int dest) {
	if (src > -1 && src < MAX_NODE)
		if (dest > -1 && dest < MAX_NODE)
			return globalState[src][dest];

	return -1;
}

void getNodeState(int src, int *myState) {
	int dest;

	if (src > -1 && src < MAX_NODE) {
		for (dest = 0; dest < MAX_NODE; dest++) {
			*(myState + dest) = globalState[src][dest];
		}
	}

	return;
}

int isReadyToRunDijkstra(void) {
	int i, j;
	
	for (i = 0; i < MAX_NODE; i++) {
		for (j = 0; j < i; j++) {
			if ( globalState[i][j] != globalState[j][i])
				return 0;
		}
	}

	return 1;
}

int zipState(char *state_c, int *state_i) {
	int i;
	int len = 0;
	short int net_i;
	int net_state_i_i;

	for (i = 0; i < MAX_NODE; i++) {
		if ( *(state_i + i) != 0) {
			//DEBUG
			//printf("zipState(): %d : %d\n", i, *(state_i + i));
			net_i = htons(i);
			net_state_i_i = htonl( *(state_i + i));
			memcpy(state_c+len, &net_i, sizeof(short int));
			memcpy(state_c+len+sizeof(short int), &net_state_i_i, sizeof(int));
			len += sizeof(short int) + sizeof(int);
		}
	}

	*(state_c + len) = '\0';

	return len;
}

int unzipState(int *state_i, char *state_c, int stateLen) {
	short int index = 0;
	int value;
	int len = 0;

	while ( len < stateLen ) {
		memcpy(&index, state_c + len, sizeof(short int));
		index = ntohs(index);

		if (index > -1 && index < MAX_NODE) {
			memcpy(&value, state_c+len+sizeof(short int), sizeof(int));
			*(state_i + index) = ntohl(value);
		}

		//DEBUG
		//printf("unzipState(): %hd : %d\n", index, *(state_i + index));

		len += sizeof(short int) + sizeof(int);
	}

	return len;
}

int scanNodeState(int src, char *state_c) {
	int state_i[MAX_NODE];
	getNodeState(src, state_i);

	return zipState(state_c, state_i);
}
/*-----------VIRTUAL NEIGHBOR TABLE-----*/
int isNeighbor(int dest) {
	if (getConnState(globalMyID, dest) > 0)
		return 1;
	else 
		return 0;
}

/* ----------COST TABLE--------------- */
int getCost(int dest) {
	return globalCostTable[dest];
}

/* ----------HEARTBEAT TABLE--------- */



////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////WRITE ONLY OPERATIONS//////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

/* ---------SEQUENCE TABLE----------- */
//This function is called everytime a non-heartbeat msg is initiated
void addSequence(int dest) {
	sequenceTable[dest]++;
}

void updateSequence(int dest, int newSeq) {
	int isNewMember = 0;

	if (newSeq > sequenceTable[dest]) {
		if (sequenceTable[dest] == 0) isNewMember = 1;
		sequenceTable[dest] = newSeq;
	}

	//Meet a new member
	if (isNewMember) {
		printf("MEET NEW MEMBER %d!\n", dest);
	}
}

void zeroSequence(int dest) {
	sequenceTable[dest] = 0;
}

/* ----------ROUTING TABLE------------- */
int getDirectLink(int dest) {
	if (dest == routingTable[dest])
		return dest;
	else return getDirectLink(routingTable[dest]);
}

void swapRoutingTable(int *newTable) {
	int dest;
	for (dest = 0; dest < MAX_NODE; dest++){
		if (routingTable[dest] > -1 && *(newTable + dest) == -1)
			zeroSequence(dest);

		routingTable[dest] = *(newTable + dest);
	}
}

void flushRoutingTable(void) {
	int i;
	for (i = 0; i < MAX_NODE; i++) {
		if (routingTable[i] != -1) {
			routingTable[i] = getDirectLink(i);
		}
	}
}

void updateRoutingTable(void) {
	int new_routingTable[MAX_NODE];

	runDijkstra();

	getDijkstraResult(new_routingTable, MAX_NODE);
	//getDijkstraResult(routingTable, MAX_NODE);

	swapRoutingTable(new_routingTable); //TODO may harm the performance

	flushRoutingTable();
	
	//DEBUG
	printRoutingTable();

	killDijkstra();
}

/* ----------GLOBAL STATE-------------- */
void updateGlobalState(int src, int dest, int cost) {
	if (globalState[src][dest] == cost) return;
	
	globalState[src][dest] = cost;
	
	if (src == globalMyID) 
		addSequence(globalMyID);

	updateRoutingTable();
}

void updateGlobalStateByRow(int src, int *state) {
	int isNewState = 0;
	int dest;

	for (dest = 0; dest < MAX_NODE; dest++) {
		if (globalState[src][dest] != state[dest]) {
			if (!isNewState) isNewState = 1;
			globalState[src][dest] = state[dest];
		}
	}
	
	if (isNewState) {
		addSequence(globalMyID);
		updateRoutingTable();
	}
}

/*-----------VIRTUAL NEIGHBOR TABLE---------*/
void addNewNeighbor(int dest) {
	updateGlobalState(globalMyID, dest, getCost(dest));
}

void modifyNeighbor(int dest, int newCost) {
	updateGlobalState(globalMyID, dest, newCost);
}

void removeNeighbor(int dest) {
	updateGlobalState(globalMyID, dest, 0);
}

int updateNeighbor(void) {
	struct timeval curTime;
	int dest;

	gettimeofday(&curTime, 0);

	for (dest = 0; dest < MAX_NODE; dest++) {
		if (isNeighbor(dest)) {
			if ( difftime(curTime.tv_sec, globalLastHeartbeat[dest].tv_sec) > TIME_OUT) {
				removeNeighbor(dest);
			}
		}
	}
}

/* ----------COST TABLE--------------- */
void updateCost(int dest, int newCost) {
	globalCostTable[dest] = newCost;
	if (isNeighbor(dest)) {
		modifyNeighbor(dest, newCost);
	}
}

/* ----------HEARTBEAT TABLE--------- */
void updateHeartbeat(int dest) {
	gettimeofday(&globalLastHeartbeat[dest], 0);
}
