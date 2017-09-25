#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <glib.h>
#include <sys/time.h>
#include <errno.h>

#define MESSAGE_SIZE 512
#define MAX_CLIENTS 20

/* Function definitions go here */

int main(int argc, char *argv[])
{
	int sockfd;
	int connectionFd;
	int portNo;
	int isPersistent = TRUE;
	struct sockaddr_in server, client;
	char message[MESSAGE_SIZE];

	// Get the port number given in arguments
	// The number of arguments should be 1 or more?
	if(argc < 2){
		printf("Fail: Please provide a port as an argument. %s\n", strerror(errno));
		return -1;
	}
	else{
		portNo = atoi(argv[1]);
	}


	// Create and bind a TCP socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Failed to create the socket. %s\n", strerror(errno));
		return -1;
	}	
    // Network functions need arguments in network byte order instead
    // of host byte order. The macros htonl, htons convert the
	// values.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(atoi(argv[1]));
	int success = bind(sockfd, (struct sockaddr *)&server, (socklen_t)sizeof(server));
	if(success == -1){
		printf("Failed to bind the socket. %s\n", strerror(errno));
		return -1;
	}	

	// Listen for connections, max 128 connections (128 cause good practice)
	if(listen(sockfd, 128) == -1){
		printf("Failed listening for connections. %s\n", strerror(errno));
	}

	for (;;) {
		int acceptFd;
		// initialize timeout
		struct timeval timeout;
		timeout.tv_sec = 30;
		
		socklen_t len = (socklen_t)sizeof(client);
		acceptFd = accept(sockfd, (struct sockaddr *) &client, &len);
		if(acceptFd > 0){
			printf("Failed to accept connection. %s\n", strerror(errno));
		}

		ssize_t n = recvfrom(acceptFd, message, sizeof(message) - 1, 0, (struct sockaddr *)&client, &len);
		// Failed to receive from socket
        if(n < 0){
			printf("Failed to receive data from the socket %s\n", strerror(errno));
		}
		// Add a null-terminator to the message
		message[n] = '\0';
		//Remove this!
		fprintf(stdout, "Received:\n%s\n", message);
		
		
	}

	// Listen on multiple sockets
	// TODO 

	/* PSEUDOCODE PLANNING
		Our port is: 59442
		RFC for Http 1.1: https://tools.ietf.org/html/rfc2616

		validate args
		Find port number from args
		Connect to client
		Bind a TCP socket
		Receive message request/response? from client
		Parse the header
			check if persistent
		Make Get
		Make Post
		Make Head
		Free Memory

		For each request print a single line to a log file that conforms to the format:
			timestamp : <client ip>:<client port> <request method>
			<requested URL> : <response code>


	*/
	return 0;
}
/*
	HELPER FUNCTIONS GO HERE
*/