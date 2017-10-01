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
#define HOST_URL "http://localhost"
#define DATE_TIME_SIZE 16

// TODO
/* PSEUDOCODE PLANNING
	Our port is: 59442
	RFC for Http 1.1: https://tools.ietf.org/html/rfc2616
	ssh tunnel: ssh -L 59442:127.0.0.1:59442 skulia15@skel.ru.is 

	Start server with: ./httpd 59442
	Test server in seperate teminal with command: curl -X (GET/POST/HEAD) localhost:59442
	Test with data: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS"}' localhost:59442
	scp /home/skulipardus/Háskólinn\ í\ Reykjavík/Tölvusamskipti/Http-Server-pt1/src/httpd.c  skulia15@skel.ru.is:tsam/pa2/src/httpd.c
	Testing Post: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS", "data": "someData", "Connection" : "Connect plz"}' localhost:59442


	validate args DONE
	Find port number from args DONE
	Connect to client DONE
	Connect to multiple clients in parallel
	Bind a TCP socket DONE
	Receive message request from client DONE
	Check cookie?
	Parse the header?
	Check for persistence
		
		if HTTP/1.0 -> not persistent
		IF HTTP/1.1 -> persistent
		if keep-alive != NULL -> persistent
	Handle connection based on persistence
	Handle timeout, 30 sec
	Check Operation (GET / POST/ HEAD) DONE
	Error response if method not supported
	Make Get done??
	Make Post
	Make Head
	Send Response
	check if client gets response
	Free Memory

	Close the connection
	FIX ALL WARNINGS
	FIX ALL MEMORY LEAKS
	CHECK IF MAKEFILE IS CORRECT - Compiles w/o warnings with flags given in description
	Readme should give directions on how to test the program
	(3 points) Make sure that your implementation does not crash, that memory is managed
	correctly, and does not contain any obvious security issues. Try to keep track of what comes
	from the clients, which may be malicious, and deallocate what you do not need early. Zero
	out every buffer before use, or allocate the buffer using g_new0().

	For each request print a single line to a log file that conforms to the format:
		timestamp : <client ip>:<client port> <request method>
		<requested URL> : <response code> DONE


*/
/* Function definitions go here */
void doMethod(int clientFd, gchar *methodType, gchar *protocol, 
	struct sockaddr_in *clientAddress, gchar *page, char* portNo, gchar* data);
void doGet(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo);
void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo, gchar* data);
void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo);
void makeHeader(gchar *protocol, char *header, size_t contentLen);
void makeBody(struct sockaddr_in *clientAddress, gchar *page, char* portNo, char* html, gchar* data);
void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, int portNo);
char *getIPAddress(struct sockaddr_in *clientAddress);
void getTime(char* timeStamp);
size_t my_strftime(char *s, size_t max, const char *fmt,const struct tm *tm);


int main(int argc, char *argv[])
{
	int serverFd;
	int clientFd;
	int portNo;
	char* portNoString;
	struct sockaddr_in server, client;
	char message[MESSAGE_SIZE];

	//bool isPersistent = true;

	// Get the port number given in arguments
	// The number of arguments should be 1 or more?
	if (argc < 2)
	{
		printf("Fail: Please provide a port as an argument. %s\n", strerror(errno));
		return -1;
	}
	else
	{
		portNo = strtol(argv[1], NULL, 10);
		portNoString = argv[1];
	}

	// Create and bind a TCP socket
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0)
	{
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
	if (bind(serverFd, (struct sockaddr *)&server, (socklen_t)sizeof(server)) < 0)
	{
		printf("Failed to bind the socket. %s\n", strerror(errno));
		close(serverFd);
		return -1;
	}
	// initialize timeout
	//struct timeval timeout;
	//timeout.tv_sec = 30;

	// Listen for connections, max 128 connections (128 because it's good practice?)
	// For only 1 connection at a time use 1
	if (listen(serverFd, 1) == -1)
	{
		printf("Failed listening for connections. %s\n", strerror(errno));
		close(serverFd);
		return -1;
	}

	for (;;)
	{
		socklen_t len = (socklen_t)sizeof(client);
		// Accept connection
		clientFd = accept(serverFd, (struct sockaddr *)&client, &len);
		if (clientFd < 0)
		{
			printf("Failed to accept connection. %s\n", strerror(errno));
		}

		ssize_t n = recvfrom(clientFd, message, sizeof(message) - 1, 0, (struct sockaddr *)&client, &len);
		// Failed to receive from socket
		if (n < 0)
		{
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
		while ((tmp = (strstr(tmp, "\r\n"))))
		{
			countLines++;
			tmp++;
		}

		// Split the string by newline and put them in an array
		gchar **splitString = g_strsplit(message, "\r\n", countLines);
		gchar **firstLine = g_strsplit(splitString[0], " ", sizeof(splitString[0]));
		// Find the method type (GET/POST/HEAD)
		gchar *methodType = firstLine[0];
		// Find the requested page in the request
		gchar *page = firstLine[1];
		// Find the protocol (HTTP.1.1)
		gchar *protocol = firstLine[2];

		// Data is found in request when we have seen 2 "\r\n" in a row.
		gchar **headerDataSplit = g_strsplit(message, "\r\n\r\n", countLines);
		gchar *data = headerDataSplit[1];
		if(data != NULL){
			printf("Data in Post Request: %s\n", data);
			fflush(stdout);
		}
		else{
			printf("No data found\n");
			fflush(stdout);
		}

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
		struct sockaddr_in *clientAddress = (struct sockaddr_in *)&client;
		makeLogFile(clientAddress, methodType, portNo);
		doMethod(clientFd, methodType, protocol, clientAddress, page, portNoString, data);
		g_strfreev(splitString);
		g_strfreev(firstLine);

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
	//close(serverFd);
	shutdown(clientFd, SHUT_RDWR);
	return 0;
}
void doMethod(int clientFd, gchar *methodType, gchar *protocol,
	 struct sockaddr_in *clientAddress, gchar *page, char* portNo, gchar* data)
{
	// Do method depending on the mode
	if (g_strcmp0(methodType, "GET") == 0)
	{
		// Do get
		doGet(clientFd, protocol, clientAddress, page, portNo);
	}
	else if (g_strcmp0(methodType, "POST") == 0)
	{
		// Do Post
		doPost(clientFd, protocol, clientAddress, page, portNo, data);
	}
	else if (g_strcmp0(methodType, "HEAD") == 0)
	{
		// Do Head
		doHead(clientFd, protocol, clientAddress, page, portNo);
	}
}

void doGet(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo)
{
	// The HTML for the page
	char html[DATA_SIZE];
	makeBody(clientAddress, page, portNo, html, NULL);


	// Create the header
	char header[HEADER_SIZE];
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
void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo, gchar* data)
{
	// The HTML for the page
	char html[DATA_SIZE];
	makeBody(clientAddress, page, portNo, html, data);

	// Create the header
	char header[HEADER_SIZE];
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
void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char* portNo)
{
	
	char html[DATA_SIZE];
	makeBody(clientAddress, page, portNo, html, NULL);


	// Create the header
	char header[HEADER_SIZE];
	makeHeader(protocol, header, strlen(html));

	// Send data back to client
	size_t size = sizeof(header);
	send(clientFd, header, size, 0);
}

void makeHeader(gchar *protocol, char *header, size_t contentLen)
{
	char timeStamp[DATE_TIME_SIZE];
	getTime(timeStamp);

	// https://stackoverflow.com/questions/20685080/convert-size-t-to-string
	char contentLenStr[sizeof(contentLen)];
	snprintf(contentLenStr, sizeof(contentLen), "%zu", contentLen);

	/*if(g_strcmp0(protocol, "HTTP/1.0")){
		persistance = false;
	}*/

	memset(header, 0, HEADER_SIZE * sizeof(char));
	// Fill header with values
	strcat(header, protocol);
	strcat(header, " 200 OK\r\n");
	strcat(header, "Date: ");
	strcat(header, timeStamp);
	strcat(header, "\r\n");
	strcat(header, "Server: ");
	strcat(header, "Great server name /v1.1/\r\n");
	strcat(header, "Content-Length: ");
	strcat(header, contentLenStr);
	strcat(header, "\r\n");
	strcat(header, "Content-Type: text/html\n");
	strcat(header, "Connection: ");
	strcat(header, " Closed\r\n");
	// Do indicate that the header field is done
	strcat(header, "\r\n");

	/*if(persistance == false){
		strcat(header, "Closed\n");
	}*/
}

void makeBody(struct sockaddr_in *clientAddress, gchar *page, char* portNo, char* html, gchar* data)
{
	memset(html, 0, DATA_SIZE);
	
	// Open of HTML
	strcat(html, "<!DOCTYPE html><html><body><h1>");

	// Add this to the html URL/RequestedPage ClientIP:Portnumber
	char content[DATA_SIZE];
	memset(content, 0, sizeof(content));
	strcat(content, HOST_URL);
	strcat(content, page);
	strcat(content, " ");
	strcat(content, getIPAddress(clientAddress));
	strcat(content, ":");
	strcat(content, portNo);

	// Add content to HTML
	strcat(html, content);
	if(data != NULL){
		strcat(html, data);
	}

	// Close HTML
	strcat(html, "</h1></body></html>");
}

void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, int portNo)
{
	FILE *logFile;
	/*
	Logger Format:
	timestamp : <client ip>:<client port> <request method> <requested URL> : <response code>
	*/

	// Logger
	logFile = fopen("httpd.log", "a");
	if (logFile == NULL)
	{
		printf("Failed to create logfile %s\n", strerror(errno));
	}

	// Get current time
	char timeStamp[DATE_TIME_SIZE];
	getTime(timeStamp);

	char IPClientStr[INET_ADDRSTRLEN];
	memset(IPClientStr, 0, sizeof(IPClientStr));
	strcpy(IPClientStr, getIPAddress(clientAddress));

	// Add text to logfile
	// Print time
	fprintf(logFile, "%s : ", timeStamp);
	// Print IP of client
	fprintf(logFile, "%s:", IPClientStr);
	// Print port number
	fprintf(logFile, "%d ", portNo);
	// Print request method
	fprintf(logFile, "%s ", methodType);
	// Print requested URL
	fprintf(logFile, "%s:%d : ", HOST_URL, portNo);
	// Print response code
	// TODO: get actual response code? priority: low
	fprintf(logFile, "200 OK\n");
	// Close the log file
	fclose(logFile);
}

char *getIPAddress(struct sockaddr_in *clientAddress)
{
	// https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
	// Get the ip address of a client
	struct in_addr ipAddr = clientAddress->sin_addr;
	char IPClientStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipAddr, IPClientStr, INET_ADDRSTRLEN);

	return IPClientStr;
}

void getTime(char* timeStamp){
	// https://stackoverflow.com/questions/1442116/how-to-get-date-and-time-value-in-c-program/30759067#30759067
	// Configure time to the current time
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	strftime(timeStamp, sizeof(timeStamp), "%c", tm);
}

// Fix for a bug in gcc that complains that %c yields only last 2 digits of year in some locales.
// size_t my_strftime(char *s, size_t max, const char *fmt,const struct tm *tm)
// {
// 	return strftime(s, max, fmt, tm);
// }