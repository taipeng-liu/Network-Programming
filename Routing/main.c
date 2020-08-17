#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "src/monitor_neighbors.c"

#define PORT 7777
#define BUF_SIZE 100
#define MAX_NODE 256

void listenForNeighbors();
void* announceToNeighbors(void* unusedParam);

/* GLOBAL PARAMETERS */
int globalMyID = 0; //Node ID
int globalSocketUDP; //UDP socket (10.1.1.nodeID:PORT)
FILE *globalLogFp = NULL;
struct sockaddr_in globalNodeAddrs[MAX_NODE];

/* GLOBAL VARIABLES */
int globalCostTable[MAX_NODE];
struct timeval globalLastHeartbeat[MAX_NODE];
int globalState[MAX_NODE][MAX_NODE]; //globalState[Src][Neighbor] = cost
int sequenceTable[MAX_NODE]; //record latest sequence # of each node
int routingTable[MAX_NODE]; //routingTable[dest ID] = nexthop ID

/* MAIN */ 
int main(int argc, char** argv)
{
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s mynodeid initialcostsfile logfile\n\n", argv[0]);
		exit(1);
	}

	FILE *costFp;
	char buf[BUF_SIZE];
	int nodeID, cost, i;
	struct sockaddr_in bindAddr;
	pthread_t hbBroadcaster, hbChecker, LspSender;
	
	//Init global node Id
	globalMyID = atoi(argv[1]);
	
	//Init global variables
	for(i = 0; i < MAX_NODE; i++)
	{
		gettimeofday(&globalLastHeartbeat[i], 0);
		globalCostTable[i] = 1;
		routingTable[i] = -1;
		
		char tempaddr[100];
		sprintf(tempaddr, "10.1.1.%d", i);
		memset(&globalNodeAddrs[i], 0, sizeof(globalNodeAddrs[i]));
		globalNodeAddrs[i].sin_family = AF_INET;
		globalNodeAddrs[i].sin_port = htons(PORT);
		inet_pton(AF_INET, tempaddr, &globalNodeAddrs[i].sin_addr);
	}

	//Update globalCostTable from cost file
	if ( (costFp = fopen(argv[2], "r")) )
	{
		memset(buf, 0, BUF_SIZE);
		while(fgets(buf, BUF_SIZE, costFp) != NULL) {
			sscanf(buf, "%d %d", &nodeID, &cost);
			if (nodeID > -1 && nodeID < MAX_NODE) {
				globalCostTable[nodeID] = cost;
			}

			memset(buf, 0, BUF_SIZE);
		}
	} else {
		perror("costFile_fd: ");
	}
	
	//Init log file descriptor
	if ( !(globalLogFp = fopen(argv[3], "w")) )
	{
		perror("globalLogFp: ");
		exit(1);
	}

	//socket()	
	if((globalSocketUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}

	//bind()
	memset(buf, 0, BUF_SIZE);
	sprintf(buf, "10.1.1.%d", globalMyID);

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, buf, &bindAddr.sin_addr);

	if(bind(globalSocketUDP, (struct sockaddr*)&bindAddr, sizeof(struct sockaddr_in)) < 0)
	{
		perror("bind");
		close(globalSocketUDP);
		exit(1);
	}
	
	pthread_create(&hbBroadcaster, 0, announceToNeighbors, (void*)0);
	pthread_create(&hbChecker, 0, checkCurNeighbors, (void*)0);
	pthread_create(&LspSender, 0, lspSendTimer, (void*)0);
	
	listenForNeighbors();
}
