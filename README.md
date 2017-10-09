# HTTP-Server-P1
## Authors: 
#### Skúli Arnarsson <skulia15@ru.is>
#### Darri Valgarðsson <darriv15@ru.is>
#### Axel Björnsson <axelb15@ru.is>

## Overview:
A Hypertext Transfer Protocol server that allows 

Compile with:  `make -C ./src`

To run the server use: `./src/httpd PORT_NUMBER`

GET, POST and HEAD requests are sent at `http://localhost:PORT_NUMBER`

## Description of Functions:

##### void closeFpIfOpen();
This function closes the file pointer if it is open. This function is usually called when the program encounters an error. This prevents memory leaks.

##### void doMethod(int clientFd, gchar *methodType, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, bool persistence);
This function compares the method value of the header. If the given value matches a supported method the matches' corresponding method is initiated. The function handles errors if a given method is not supported.

##### void doGet(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, bool persistence);
This function handles GET requests and makes a response with HTML and a header, and sends it. Calls the logger to log the response.  It uses makeHeader, makeBody and makeLogFile to do so.

##### void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, bool persistence);
This function handles POST requests and makes a response with HTML and a header, and sends it. Calls the logger to log the response. It uses makeHeader, makeBody and makeLogFile to do so.

##### void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, bool persistence);
This function handles HEAD requests and makes a response with a header, and sends it. Calls the logger to log the response. It uses makeHeader to do so.

##### void makeHeader(gchar *protocol, char *header, size_t contentLen, bool persistence);
This function makes a header out of the given parameters. The function results in a modified *header.

##### void makeBody(struct sockaddr_in *clientAddress, gchar *page, char *portNo, char *html, gchar *data);
This function modifies *html into a finalised body, which can be used by other functions.

##### void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, char *portNo, char *statusCode);
This function makes and/or opens a log file and logs requests into it.

##### char *getIPAddress(struct sockaddr_in *clientAddress);
This function returns the IP address of the client issuing a request.

##### char *getTime();
This function returns a string with the current date and time.

##### bool checkPersistence(gchar *header, char *protocol);
Returns a boolean value indicating if the connection requests a persistent connection or not where true is returned if the connection is persistant.

##### void handleUnsupported(int clientFd, gchar *methodType, gchar *protocol, struct sockaddr_in *clientAddress, char *portNo);
Create an error response which is sent to the client if the request contains a method this is not supported by the server. Response code 405 for Method Not Allowed.