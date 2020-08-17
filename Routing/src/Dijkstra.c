#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_NODE 256

/* GLOBAL PARAMETERS FROM main.c */
extern int globalMyID;

/* FUNCTION FROM update.c */
extern int getConnState(int src, int dest);

/* LINKED LIST */
struct node {
	int dest;
	int cost;
	int nextHop;

	struct node *next;
};

struct node *tentative = NULL;
struct node *confirmed = NULL;

/* -------------appearance--------- */
void printList(struct node *head) {
	struct node *ptr = head;

	printf("|Dest, Cost, NextHop|\n");
	while (ptr) {
		printf("|   %d,   %d,   %d   |\n", ptr->dest, ptr->cost, ptr->nextHop);
		ptr = ptr->next;
	}

	printf("     (end)\n");
}

void printTentative(void) {
	printf("Tentative List:\n");
	printList(tentative);
}
void printConfirmed(void) {
	printf("Confirmed List:\n");
	printList(confirmed);
	printf("\n");
}

/* --------------find--------------- */
struct node *findInList(struct node *head, int dest) {
	struct node *ptr = head;

	while (ptr) {
		if (ptr->dest == dest) return ptr;
		ptr = ptr->next;
	}

	return NULL;
}

struct node *findInTentative(int dest) {

	return findInList(tentative, dest);
}

struct node *findInConfirmed(int dest) {

	return findInList(confirmed, dest);
}

/* --------------add---------------- */
struct node *addToList(struct node **head, int dest, int cost, int nextHop) {
	struct node *newNode = malloc(sizeof(struct node));

	newNode->dest = dest;
	newNode->cost = cost;
	newNode->nextHop = nextHop;
	newNode->next = *head;
	*head = newNode;

	//DEBUG
	//printf("parameters = {%d, %d, %d}\n", dest, cost, nextHop);
	//printf("newNode={%d, %d, %d}\n", newNode->dest, newNode->cost, newNode->nextHop);

	return newNode;
}

struct node *addToTentative(int dest, int cost, int nextHop) {
	struct node *found = findInTentative(dest);

	if (found) {
		if (cost < found->cost || ((cost == found->cost) && (nextHop < found->nextHop)) ) {
			found->cost = cost;
			found->nextHop = nextHop;
		}
	} else {
		found = addToList(&tentative, dest, cost, nextHop);
	}

	return found;
}

struct node *addToConfirmed(int dest, int cost, int nextHop) {
	return addToList(&confirmed, dest, cost, nextHop);
}

/* --------------remove node------------- */
struct node *removeFromList(struct node *head, int dest) {
	struct node *cur = head, *prev = head;
	struct node *ret = NULL;

	while (cur) {
		if (cur->dest == dest) {
			ret = cur;
			if (cur != prev) {
				prev->next = cur->next;
				cur->next = NULL;
			} else {
				head = NULL;
			}
		}

		prev = cur;
		cur = cur->next;
	}

	return ret;
}

int popTentative(int *dest, int *cost, int *nextHop) {
	struct node *cur = tentative, *prev = tentative;
	struct node *tempCur = NULL, *tempPrev = NULL;

	while (cur) {
		if (tempCur == NULL 
				|| cur->cost < tempCur->cost 
				|| ((cur->cost == tempCur->cost) && (cur->nextHop < tempCur->nextHop))) {
			tempCur = cur;
			tempPrev = prev;
		}
		prev = cur;
		cur = cur->next;
	}

	if (tempCur) {
		if (tempCur != tentative) {
			tempPrev->next = tempCur->next;
			tempCur->next = NULL;
		} else {
			tentative = tempCur->next;
			tempCur->next = NULL;
		}

		*dest = tempCur->dest;
		*cost = tempCur->cost;
		*nextHop = tempCur->nextHop;
		free(tempCur);

		return 0;
	}

	return -1;
}


/* -------------delete list--------- */
void deleteList(struct node **head) {
	struct node *ptr = *head;
	struct node *toDelete;

	while (ptr){
		toDelete = ptr;
		ptr = ptr->next;

		free(toDelete);
	}

	*head = NULL;
}

/* --------------APIs--------------- */
void runDijkstra(void) {	
	int src = globalMyID;
	int addCost = 0;
	int dest, cost, nextHop;

	int cdest, ccost, cnextHop;

	do {
		if ( popTentative(&cdest, &ccost, &cnextHop) > -1){
			addToConfirmed(cdest, ccost, cnextHop);
			src = cdest;
			addCost = ccost;
		}

		//DEBUG
		//printTentative();
		//printConfirmed();

		for (dest = 0; dest < MAX_NODE; dest++){
			cost = getConnState(src, dest);
	  		
			if (cost != 0 && dest != globalMyID && (findInConfirmed(dest) == NULL)){	
				if (src == globalMyID) 
					nextHop = dest;
	  			else 
					nextHop = src;
				addToTentative(dest, cost + addCost, nextHop);
			}
		}
		
		//DEBUG
		//printTentative();
		//printConfirmed();

	}while(tentative);
}

void getDijkstraResult(int *rt, int len) {
	struct node *ptr = confirmed;
	int i;

	for (i = 0; i < len; i++) {
		*(rt + i) = -1;
	}

	while (ptr) {
		*(rt + ptr->dest) = ptr->nextHop;
		ptr = ptr->next;
	}
}

void killDijkstra(void) {
	deleteList(&confirmed);
	deleteList(&tentative);
}
