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
#include <time.h>
#include <errno.h>

#define MESSAGE_SIZE 1024
#define DATA_SIZE 512
#define MAX_CLIENTS 20
#define HEADER_SIZE 512

// TODO 
/* PSEUDOCODE PLANNING
	Our port is: 59442
	RFC for Http 1.1: https://tools.ietf.org/html/rfc2616

	Start server with: ./httpd 59442
	Test server in seperate teminal with command: curl -X (GET/POST/HEAD) localhost:59442
	Test with data: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS"}' localhost:59442
	scp /home/skulipardus/Háskólinn\ í\ Reykjavík/Tölvusamskipti/Http-Server-pt1/src/httpd.c  skulia15@skel.ru.is:tsam/pa2/src/httpd.c

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
/* Function definitions go here */
void doMethod(int clientFd, gchar* methodType, gchar* protocol);
void doGet(int clientFd, gchar* protocol);
void doPost(int clientFd);
void doHead(int clientFd, gchar* protocol);
void makeHeader(gchar* protocol, char* header, size_t contentLen);

int main(int argc, char *argv[]){
	int serverFd;
	int clientFd;
	int portNo;
	struct sockaddr_in server, client;
	char message[MESSAGE_SIZE];
	//bool isPersistent = true;
	
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
	// initialize timeout
	//struct timeval timeout;
	//timeout.tv_sec = 30;
	
	// Listen for connections, max 128 connections (128 because it's good practice?)
	if(listen(serverFd, 128) == -1){
		printf("Failed listening for connections. %s\n", strerror(errno));
		close(serverFd);
		return -1;
	}

	for (;;) {
		socklen_t len = (socklen_t)sizeof(client);
		// Accept connection
		clientFd = accept(serverFd, (struct sockaddr *) &client, &len);
		if(clientFd < 0){
			printf("Failed to accept connection. %s\n", strerror(errno));
		}
		fprintf(stdout, "Client has connected\n\n");

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
		fprintf(stdout, "Request:\n%s\n\n", message);

		// Count the number of lines in the string
		// Important! https://stackoverflow.com/questions/9052490/find-the-count-of-substring-in-string
		const char *tmp = message;
		int countLines = 0;
		// "\r\n" is a newline character 
		while((tmp = (strstr(tmp, "\r\n")))){
			countLines++;
			tmp++;
		}

		// Split the string by newline and put them in an array
		gchar**splitString = g_strsplit(message, "\r\n", countLines);
		gchar** firstLine = g_strsplit(splitString[0], " / ", sizeof(splitString[0]));
		// Find the method type (GET/POST/HEAD)
		gchar* methodType = firstLine[0];
		// Find the protocol (HTTP.1.1) 
		gchar* protocol = firstLine[1];
		// Find the value of the headers of the request
		/*
		gchar** headerFields;
		int i;
		for(i = 1; i < countLines; i++){
			//fprintf(stdout, "Line %d: %s\n", i, splitString[i]);
			// Split each line into headerFields and value
			headerFields = g_strsplit(splitString[i], ": ", sizeof(splitString[i]));
		}
		*/

		doMethod(clientFd, methodType, protocol);

		// Check for persistence
		/*
		if(persistent){
			go on
		}
		else{
			close the connection
			close(clientFd);
		}
		*/
	}
	close(clientFd);
	close(serverFd);
	return 0;
}
void doMethod(int clientFd, gchar* methodType, gchar* protocol){
	// Do method depending on the mode
		if( g_strcmp0(methodType, "GET") == 0){
			// Do get
			doGet(clientFd, protocol);
		}
		else if(g_strcmp0(methodType, "POST") == 0){
			// Do Post
			doPost(clientFd);
		}
		else if(g_strcmp0(methodType, "HEAD") == 0){
			// Do Head
			doHead(clientFd, protocol);
		}
}

void doGet(int clientFd, gchar* protocol){
	// The HTML for the page
	char html[DATA_SIZE];
	memset(html, 0, sizeof(html));
	strcat(html, "<!DOCTYPE html><html><body><h1>Hello, World!</h1></body></html>");
	// Create the header
	char header[HEADER_SIZE];
	printf("B4$*$*$*$$*$**$*$*$\n");
	fflush(stdout);
	makeHeader(protocol, header, strlen(html));
	char response[MESSAGE_SIZE];
	memset(response, 0, sizeof(response));

	strcat(response, header);
	strcat(response, html);

	fprintf(stdout, "Response: \n%s", response);
	fflush(stdout);
	// Send data back to client
	send(clientFd, response, sizeof(response), 0);

}
void doPost(int clientFd){
	// Send data back to client
	size_t size = strlen("Post Received\n");
	send(clientFd, "Post Received\n", size, 0);
	
}
void doHead(int clientFd, gchar* protocol){
	// Create the header
	char header[HEADER_SIZE];
	makeHeader(protocol, header, 0);

	// Send data back to client
	size_t size = strlen(header);
	send(clientFd, header, size, 0);
}

void makeHeader(gchar* protocol, char* header, size_t contentLen){
	// https://stackoverflow.com/questions/1442116/how-to-get-date-and-time-value-in-c-program/30759067#30759067
	// Configure time to the current time
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char timeStamp[64];
	strftime(timeStamp, sizeof(timeStamp), "%c", tm);

	
	// https://stackoverflow.com/questions/20685080/convert-size-t-to-string
	char contentLenStr[sizeof(contentLen)];
	snprintf(contentLenStr, sizeof(contentLen), "%zu", contentLen);
	
	/*if(g_strcmp0(protocol, "HTTP/1.0")){
		persistance = false;
	}*/
	
	printf("AFTER$*$*$*$$*$**$*$*$\n");
	fflush(stdout);
	memset(header, 0, HEADER_SIZE * sizeof(char));
	// Fill header with values	
	strcat(header, protocol);
	strcat(header, " 200 OK\n");
	strcat(header, "Date: ");
	strcat(header, timeStamp);
	strcat(header, "\n");
	strcat(header, "Server: ");
	strcat(header, "Great server name /v1.1/\n");
	strcat(header, "Content-Length: ");
	strcat(header, contentLenStr);
	strcat(header, "\n");
	strcat(header, "Content-Type: text/html\n");
	strcat(header, "Connection: ");
	strcat(header, " Closed\n");
	strcat(header, "\r\n");
	/*if(persistance == false){
		strcat(header, "Closed\n");
	}*/
}
/*
void log(){

}
*/
/*
	HELPER FUNCTIONS GO HERE
*/