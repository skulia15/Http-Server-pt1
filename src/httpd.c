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
	int serverFd;
	int clientFd;
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
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverFd < 0){
		printf("Failed to create the socket. %s\n", strerror(errno));
		return -1;
	}	
    // Network functions need arguments in network byte order instead
    // of host byte order. The macros htonl, htons convert the
	// values.
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(portNo);
	if(bind(serverFd, (struct sockaddr *)&server, (socklen_t)sizeof(server)) < 0){
		printf("Failed to bind the socket. %s\n", strerror(errno));
		close(serverFd);
		return -1;
	}	

	// Listen for connections, max 128 connections (128 because it's good practice?)
	if(listen(serverFd, 128) == -1){
		printf("Failed listening for connections. %s\n", strerror(errno));
		close(serverFd);
		return -1;
	}

	for (;;) {
		// initialize timeout
		struct timeval timeout;
		timeout.tv_sec = 30;
		
		socklen_t len = (socklen_t)sizeof(client);
		clientFd = accept(serverFd, (struct sockaddr *) &client, &len);
		if(clientFd < 0){
			printf("Failed to accept connection. %s\n", strerror(errno));
		}
		fprintf(stdout, "Client has connected\n");

		ssize_t n = recvfrom(clientFd, message, sizeof(message) - 1, 0, (struct sockaddr *)&client, &len);
		// Failed to receive from socket
        if(n < 0){
			printf("Failed to receive data from the socket %s\n", strerror(errno));
			close(clientFd);
			close(serverFd);
			return -1;
		}
		// Add a null-terminator to the message
		message[n] = '\0';
		//Remove this!
		fprintf(stdout, "Received:\n%s\n", message);
		
		size_t size = strlen("Hey, Here is your data\n");
		send(clientFd, "Hey, Here is your data\n", size, 0);
		
	}
		close(clientFd);
		close(serverFd);

	// Listen on multiple sockets
	// TODO 

	/* PSEUDOCODE PLANNING
		Our port is: 59442
		RFC for Http 1.1: https://tools.ietf.org/html/rfc2616

		Start server with: ./httpd 59442
		Test server in seperate teminal with command: curl -X (GET/POST/HEAD) localhost:59442

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
		Close the connection

		For each request print a single line to a log file that conforms to the format:
			timestamp : <client ip>:<client port> <request method>
			<requested URL> : <response code>


	*/
	return 0;
}
/*
	HELPER FUNCTIONS GO HERE
*/