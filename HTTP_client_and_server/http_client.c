#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#define MAX_BUFFER_SIZE 256
#define MAX_PATH_LENGTH 100

typedef struct {
	char hostname[MAX_PATH_LENGTH];
	char port[10];
	char filepath[MAX_PATH_LENGTH];
} request_info;


int parseArg(char *arg, request_info *ri) {
	char *p0 = arg, *p1, *p2;


	if ((strstr(arg, "http://")) != NULL) {
		p0 += 7;
	} else if ((strstr(arg, "https://")) != NULL) {
		p0 += 8;
	}

	memset(ri->hostname, '\0', MAX_PATH_LENGTH);
	memset(ri->filepath, '\0', MAX_PATH_LENGTH);
	memset(ri->port, '\0', 10);

	strcpy(ri->port, "80");
	strcpy(ri->hostname, p0);

	p1 = strstr(p0, "/");
	p2 = strstr(p0, ":");

	if (p2 != NULL) {
		if (p1 != NULL) {
			if (p2 < p1) {
				ri->hostname[p2 - p0] = '\0';
				strcpy(ri->port, p2 + 1);
				ri->port[p1 - p2 - 1] = '\0';
			} else {
				ri->hostname[p1 - p0] = '\0';
			}
			strcpy(ri->filepath, p1);
		} else {
			ri->hostname[p2 - p0] = '\0';
			strcpy(ri->port, p2 + 1);
			ri->filepath[0] = '/';
		}
	} else {
		if (p1 != NULL) {
			ri->hostname[p1 - p0] = '\0';
			strcpy(ri->filepath, p1);
		} else {
			ri->filepath[0] = '/';
		}
	}

	return 0;
}

int setupClient(char *hostname, char *port) {

	int sockfd, s, opt = 1;
	struct addrinfo hints, *result, *p;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; //AF_INET for IPv4
	hints.ai_socktype = SOCK_STREAM; //Stream socket

	s = getaddrinfo(hostname, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	if (result == NULL) return -1;

	for (p = result; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
			continue;

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			fprintf(stderr, "setsockopt: SO_REUSEADDR\n");
			exit(1);
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
			fprintf(stderr, "setsockopt: SO_REUSEPORT\n");
			exit(1);
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		freeaddrinfo(result);
		return sockfd;
	}

	freeaddrinfo(result);
	return -1;
}

int scanHttpCode(char *httpResp) {
	if (strlen(httpResp) == 0) 
		return -1;
	char buf[4];
	memcpy(buf, httpResp + 9, 3);
	buf[3] = '\0';
	return atoi(buf);
}

int main(int argc, char *argv[]) {
	int sockfd;
	char buffer[MAX_BUFFER_SIZE] = {0};
	ssize_t nread;
	request_info ri;
	
	char *ptr;
	int resType, isMsgBody = 0, needRedirect = 0;
	FILE *fptr;


	if (argc != 2) {
		fprintf(stdout, "Usage: %s http://hostname[:port]/path/to/file", argv[0]);
		return 1;
	}

	parseArg(argv[1], &ri);

	//Check the result of parse_arg
	fprintf(stdout, "HOSTNAME:%s\nPORT:%s\nFILEPATH:%s\n", ri.hostname, ri.port, ri.filepath);

	//Setup client to <ri.hostname>:<ri.port>
	sockfd = setupClient(ri.hostname, ri.port);
	if (sockfd == -1) {
		fprintf(stderr, "Failed to connect to: %s:%s\n", ri.hostname, ri.port);
		exit(1);
	}

	//Send an HTTP GET request, based on the filepath in argv[1]
	if (strcmp(ri.port, "80")) {
		sprintf(buffer, "GET %s HTTP/1.0\r\nHost: %s:%s\r\n\r\n", ri.filepath, ri.hostname, ri.port);
	} else {
		sprintf(buffer, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", ri.filepath, ri.hostname);
	}
	//printf("===Send request: %s\n", buffer);
	if (send(sockfd, buffer, strlen(buffer), 0) < 1) {
		perror("send");
	}

	//Receive the response
	memset(buffer, 0, MAX_BUFFER_SIZE);
	if ((nread = recv(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0)) < 0) {
		fprintf(stderr, "No msg read from server\n");
		exit(1);
	}

	resType = scanHttpCode(buffer);

	//switch according to response type
	switch (resType) {
		case 200:
			while(nread > 0) {
				if (isMsgBody) {
					fprintf(fptr, "%s", buffer);
				} else {
					if ((ptr = strstr(buffer, "\r\n\r\n")) != NULL) {
						isMsgBody = 1;
						if ((fptr = fopen("./output", "w")) == NULL) {
							perror("fopen");
							exit(1);
						}
						//printf("%s\n", buffer);
						fprintf(fptr, "%s", ptr + 4);
					}
				}

				memset(buffer, 0, MAX_BUFFER_SIZE);
				//TODO recv block! (time out??)
				nread = recv(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0);
			}

			fclose(fptr);
			break;
		case 301:
			needRedirect = 1;

			//TODO Consider string "Location: <new_url>" in seperate buffers!!
			while(nread >= MAX_BUFFER_SIZE - 1) {
				//Find string "Location: <new_url>"
				if ((ptr = strstr(buffer, "Location: ")) != NULL) {
					strcpy(ri.hostname, ptr + 10);
					if ((ptr = strstr(ri.hostname, "\r\n")) != NULL) {
						ri.hostname[ptr - ri.hostname] = '\0';
						break;
					}
				}

				memset(buffer, 0, MAX_BUFFER_SIZE);
				nread = recv(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0);
			}
			break;
		default:
			printf("Response from server: %s", buffer);
	}
	//printf("====Complete\n");
	//Close connection and return
	close(sockfd);

	//Check if redirection is needed
	if (needRedirect) {
		char *new_argv[] ={argv[0], ri.hostname, NULL};
		printf("HTTP ERROR 301. Need to redirect to new location: %s\n", ri.hostname);

		//Execute new process
		execvp(new_argv[0], new_argv);
	}

	return 0;
}
