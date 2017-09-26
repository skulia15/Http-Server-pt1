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
#include <stdbool.h>
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
	// Bind to socket
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
		// Accept connection
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

		// Count the number of lines in the string
		// Important! https://stackoverflow.com/questions/9052490/find-the-count-of-substring-in-string
		
		const char *tmp = message;
		int countLines = 0;
		// "\r\n" is a newline character 
		while((tmp = (strstr(tmp, "\r\n")))){
			countLines++;
			tmp++;
		} 
		fprintf(stdout, "Number of lines is: %d\n", countLines);

		// Split the string by newline and put them in an array
		gchar **splitString = g_strsplit(message, "\r\n", countLines);
		gchar** dataField;
		gchar** value;
		int i;
		for(i = 0; i < countLines; i++){
			fprintf(stdout, "Line %d: %s\n", i, splitString[i]);
			// Split each line into dataField and value
			dataField = g_strsplit(splitString[i], ": ", sizeof(splitString[i]));
			fprintf(stdout, "Data Field: %s \n", dataField[0]);
			// Do operation depending on the mode
			if(strstr(dataField[0], "GET") != NULL){
				fprintf(stdout, "GET == %s \n",  dataField[0]);
				// Do get
			}
			else if(strstr(dataField[0], "POST") != NULL){
				fprintf(stdout, "POST == %s \n", dataField[0]);
				// Do Post
			}
			else if(strstr(dataField[0], "HEAD") != NULL){
				fprintf(stdout, "HEAD == %s \n", dataField[0]);
				// Do Head
			}

		}
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
		Test with data: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS"}' localhost:59442

		validate args X
		Find port number from args X
		Connect to client X
		Connect to multiple clients in parallel
		Bind a TCP socket X
		Receive message request from client X
		Check cookie
		Parse the header
		Check for persistence
			if HTTP/1.0 -> not persistent
		Check Operation (GET / POST/ HEAD)
		Make Get
		Make Post
		Make Head
		Send Response
		Send cookie
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