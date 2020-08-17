#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PACKET_SIZE 1472
#define HEADER_LEN 19
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-HEADER_LEN

/* Instrcution of myPakcet
 * Total length must be no larger than 1472
 *
 * Msg type: 1 - normal packet
 * 	     0 - ACK
 *
 * Msg format:
 * 	Msg type(2) + File No.(4) + seq(4) + offset(4) + isLast(1) + clen(4) + content(...)
 *
 */

struct myMsg {
	short int type;
	int fileNo;
	int seq;
	int offset;
	char isLast;
	int clen;
	char content[MAX_CONTENT_SIZE];
};

int formMyMsg(struct myMsg *mmsg, short int type, int fileNo, int seq, 
		int offset, char isLast, int clen, char *content) 
{
	if (clen > MAX_CONTENT_SIZE) {
		fprintf(stderr, "newNornalMsg(): Content is too large\n");
		return 0;
	}

	mmsg->type = type;
	mmsg->fileNo = fileNo;
	mmsg->seq = seq;
	mmsg->offset = offset;
	mmsg->isLast = isLast;
	mmsg->clen = clen;
	memcpy(mmsg->content, content, clen);

	return HEADER_LEN + clen;
}

int myMsgToBytes(char *bytes, struct myMsg *m) {
	int ret = 0;
	short int type = htons(m->type);
	int fileNo = htons(m->fileNo);
	int seq = htons(m->seq);
	int offset = htons(m->offset);
	char isLast = m->isLast;
	int clen = htons(m->clen);

	memcpy(bytes + ret, &type, sizeof(short int));
	ret += sizeof(short int);
	memcpy(bytes + ret, &fileNo, sizeof(int));
	ret += sizeof(int);
	memcpy(bytes + ret, &seq, sizeof(int));
	ret += sizeof(int);
	memcpy(bytes + ret, &offset, sizeof(int));
	ret += sizeof(int);
	memcpy(bytes + ret, &isLast, sizeof(char));
	ret += sizeof(char);
	memcpy(bytes + ret, &clen, sizeof(int));
	ret += sizeof(int);
	memcpy(bytes + ret, m->content, m->clen);
	ret += m->clen;

	return ret;
}

int bytesToMyMsg(struct myMsg *m, char *bytes) {
	int ret = 0;

	memcpy(&(m->type), bytes + ret, sizeof(short int));
	m->type = ntohs(m->type);
	ret += sizeof(short int);
	memcpy(&(m->fileNo), bytes + ret, sizeof(int));
	m->fileNo = ntohs(m->fileNo);
	ret += sizeof(int);
	memcpy(&(m->seq), bytes + ret, sizeof(int));
	m->seq = ntohs(m->seq);
	ret += sizeof(int);
	memcpy(&(m->offset), bytes + ret, sizeof(int));
	m->offset = ntohs(m->offset);
	ret += sizeof(int);
	memcpy(&(m->isLast), bytes + ret, sizeof(char));
	ret += sizeof(char);
	memcpy(&(m->clen), bytes + ret, sizeof(int));
	m->clen = ntohs(m->clen);
	ret += sizeof(int);

	memcpy(m->content, bytes + ret, m->clen);
	ret += m->clen;

	return ret;
}

