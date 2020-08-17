#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 256

#define MAX_PATH_LENGTH 256

#define BACKLOG 10 //# of connections

int setupServer(char *port) {
	int sockfd, opt = 1;
	struct sockaddr_in my_addr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return -1;
	}
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		//TODO continue or break?
		perror("setsockopt");
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
		//TODO continue or break?
		perror("setsockopt");
		return -1;
	}
	

	bzero(&my_addr, sizeof(my_addr));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(atoi(port));
	my_addr.sin_addr.s_addr = INADDR_ANY;


	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		return -1;
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("bind");
		return -1;
	}

	printf("===Listen on port %s...\n", port);

	return sockfd;
}

int main(int argc, char *argv[]){
	int sockfd, new_fd;
	struct sockaddr_in their_addr;
	int sin_size, nread, pathLen, nsend;
	char buffer[MAX_BUFFER_SIZE], filepath[MAX_PATH_LENGTH];
	char *typePtr, *httpPtr;
	FILE *fptr;
	int read_file;

	filepath[0] = '.';

	if (argc != 2) {
		fprintf(stdout, "Usage: %s <port>", argv[0]);
		exit(1);
	}

	if ((sockfd = setupServer(argv[1])) == -1) {
		fprintf(stderr, "setupServer Error\n");
		exit(1);
	}

	while(1) {
		//Main loop
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("===Connect to %s\n", inet_ntoa(their_addr.sin_addr));

		if(!fork()) {
			//Child
			memset(buffer, 0 ,MAX_BUFFER_SIZE);

			if ((nread = recv(new_fd, buffer, MAX_BUFFER_SIZE - 1, 0)) > 0) {
				//Server receives some bytes
				if ((typePtr = strstr(buffer, "GET")) != NULL && (httpPtr = strstr(typePtr, "HTTP/")) != NULL) {
					//Obtain filepath from request
					pathLen = httpPtr - typePtr - 5;
					memcpy(filepath + 1, typePtr + 4, pathLen);
					filepath[pathLen + 1] = '\0';

					//If no file is specified. default to "index.html"
					if (strcmp(filepath, "./") == 0) {
						strcat(filepath, "index.html");
						filepath[12] = '\0';
					}
					//printf("Request filepath is %s(end)\n", filepath);

					//Open the file
					if ((fptr = fopen(filepath, "r")) == NULL) {
						//No such file, send HTTP 404
						send(new_fd, "HTTP/1.0 404 Not Found\r\n\r\n", 26, 0);
						close(new_fd);
						perror("fopen");
						exit(1);
					}

					send(new_fd, "HTTP/1.0 200 OK\r\n\r\n", 19, 0);

					while (1) {
						//Send all file contents
						memset(buffer, 0, MAX_BUFFER_SIZE);
						read_file = fread(buffer, 1, MAX_BUFFER_SIZE - 1, fptr);
						if (read_file == MAX_BUFFER_SIZE - 1) {
							send(new_fd, buffer, MAX_BUFFER_SIZE - 1, 0);
						} else {
							if (feof(fptr)) {
								send(new_fd, buffer, read_file, 0);
								printf("Finish sending file %s.\n", filepath);
							} else {
								fprintf(stderr, "Other error\n");
							}
							break;
						}
					}

					fclose(fptr);
				} else {
					send(new_fd, "HTTP/1.0 400 Bad Request\r\n\r\n", 28, 0);
				}
			} else {
				printf("No data received. Connection may be closed.\n");
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);

		//TODO Check multiple downloads
		while(waitpid(-1, NULL, WNOHANG) > 0);
	}

	return 0;
}
