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

##### void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, bool persistence);

##### void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, bool persistence);

##### void makeHeader(gchar *protocol, char *header, size_t contentLen, bool persistence);

##### void makeBody(struct sockaddr_in *clientAddress, gchar *page, char *portNo, char *html, gchar *data);

##### void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, char *portNo, char *statusCode);

##### char *getIPAddress(struct sockaddr_in *clientAddress);

##### char *getTime();

##### size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

##### bool checkPersistence(gchar *header, char *protocol);

##### void handleUnsupported(int clientFd, gchar *methodType, gchar *protocol, struct sockaddr_in *clientAddress, char *portNo);

