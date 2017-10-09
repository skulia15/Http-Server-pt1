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
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>

#define MESSAGE_SIZE 1024
#define DATA_SIZE 512
#define MAX_CLIENTS 20
#define HEADER_SIZE 512
#define HOST_URL "http://localhost"
#define DATE_TIME_SIZE 64

// TODO
/* PSEUDOCODE PLANNING
    Our port is: 59442
    RFC for Http 1.1: https://tools.ietf.org/html/rfc2616
    ssh tunnel: ssh -L 59442:127.0.0.1:59442 skulia15@skel.ru.is
    Poll() info from: https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm

    Start server with: ./httpd 59442
    Test server in seperate teminal with command: curl -X (GET/POST/HEAD) localhost:59442
    Test with data: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS"}' localhost:59442
    scp /home/skulipardus/Háskólinn\ í\ Reykjavík/Tölvusamskipti/Http-Server-pt1/src/httpd.c  skulia15@skel.ru.is:tsam/pa2/src/httpd.c
    Testing Post: curl -H "Content-Type: application/json" -X POST -d '{"username": "KYS", "data": "someData", "Connection" : "Connect plz"}' localhost:59442

    // validate args DONE
    // Find port number from args DONE
    // Connect to client DONE
    // Connect to multiple clients in parallel
    // Bind a TCP socket DONE
    // Receive message request from client DONE
    // Parse the header?
    // Check for persistence
    //     if HTTP/1.0 -> not persistent
    //     IF HTTP/1.1 -> persistent
    //     if keep-alive != NULL -> persistent
    //     FOR ALL CONNECTION 1.1 and 1.0 if Connection: closed -> NOT persistent
    // Handle connection based on persistence
    // Handle timeout, 30 sec
    // Check Operation (GET / POST/ HEAD) DONE
    // Error response if method not supported
    // Make Get done??
    // Make Post
    // Make Head
    // Send Response
    // check if client gets response
    Free Memory
    Make sure consistent formatting
    //Close the connection
    FIX ALL WARNINGS
    FIX ALL MEMORY LEAKS
    CHECK IF MAKEFILE IS CORRECT - Compiles w/o warnings with flags given in description
    Comment all functions
    Readme should give directions on how to test the program
    (3 points) Make sure that your implementation does not crash, that memory is managed
    correctly, and does not contain any obvious security issues. Try to keep track of what comes
    from the clients, which may be malicious, and deallocate what you do not need early. Zero
    out every buffer before use, or allocate the buffer using g_new0().

    HTML CHANGE PORT 
    


*/
/* Function definitions go here */
void doMethod(int clientFd, gchar *methodType, gchar *protocol,
              struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, struct sockaddr_in client);
void doGet(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, struct sockaddr_in client);
void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, struct sockaddr_in client);
void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, struct sockaddr_in client);
void makeHeader(gchar *protocol, char *header, size_t contentLen);
void makeBody(struct sockaddr_in *clientAddress, gchar *page, char *portNo, char *html, gchar *data, struct sockaddr_in client);
void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, char *portNo, char *statusCode, struct sockaddr_in client);
char *getIPAddress(struct sockaddr_in *clientAddress);
char *getTime();
size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
bool checkPersistence(gchar *header, char *protocol);
void handleUnsupported(int clientFd, gchar *methodType, gchar *protocol, struct sockaddr_in *clientAddress, char *portNo, struct sockaddr_in client);

int main(int argc, char *argv[])
{
    int on = 1;
    // The request sent by the client
    char message[MESSAGE_SIZE];
    // The file descriptor for the server
    int serverFd;
    // The file descriptor for the client
    int clientFd = -1;
    // The port number as an integer, given from command line
    int portNo;
    // The port number as a string, given from command line
    char *portNoString;
    // Structures for handling internet addresses
    struct sockaddr_in server, client;
    // Flag for persistence of the connection, false by default
    bool persistence = false;
    // Connections fd's for poll()
    struct pollfd fds[200];
    // Timer for each connection, to see whether they have timed out
    time_t fdTimes[200];
    // Number of file descriptors
    int nfds = 1;

    // For use in loops
    int current_size = 0;
    int i, j, x;

    // Checks whether a connection has been closed, so we can optimize the array
    bool fdRemoved = false;


    // Get the port number given in arguments
    // The number of arguments should be 1 or more
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

    // Make the server file descriptor reusable
    // https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        fprintf(stdout, "setsockopt failed. %s\n", strerror(errno));
        close(serverFd);
        return -1;
    }

    // Make the server nonblocking
    if (ioctl(serverFd, FIONBIO, (char *)&on) < 0)
    {
        fprintf(stdout, "setsockopt failed. %s\n", strerror(errno));
        close(serverFd);
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


    // Listen for connections, max 32 connections
    // For only 1 connection at a time use 1
    // Mark socket as accepting connections, max 1 in listen queue
    if (listen(serverFd, 32) < 0)
    {
        printf("Failed listening for connections. %s\n", strerror(errno));
        close(serverFd);
        return -1;
    }

    // Clear the fd's
    memset(fds, 0, sizeof(fds));

    // Set up the first socket
    fds[0].fd = serverFd;
    fds[0].events = POLLIN;

    // Set the timeout on poll() as 0, so it can constantlycheck for new incoming connections, 
    // as well as check for timeouts or new requests on old connections, without having to 
    // wait a long time for poll() to finish
    int timeout = 0;
    // Main loop
    for (;;)
    {
        // Polling
        int pollVal = poll(fds, nfds, timeout);

        // Error
        if (pollVal < 0)
        {
            // Error
            printf("Poll encountered an error: %s\n", strerror(errno));
            break;
        }

        // Loop through all connected fds.
        // First loop will only contain the server.
        // First iteration of loop is always for the server.
        current_size = nfds;
        for (x = 0; x < current_size; x++)
        {
            if (fds[x].revents != POLLIN && fds[x].revents != 0)
            {
                continue;
            }

            if (fds[x].fd == serverFd)
            {
                // For the server, accept all incoming connections and add them to the array.
                printf("  Listening socket is readable\n");
                do
                {
                    socklen_t len = (socklen_t)sizeof(client);

                    clientFd = accept(serverFd, (struct sockaddr *)&client, &len);

                    // Error
                    if (clientFd < 0)
                    {
                        // No more connections to be accepted
                        break;
                    }
                    fds[nfds].fd = clientFd;
                    fds[nfds].events = POLLIN;
                    time(&fdTimes[nfds]);
                    nfds++;
                } while (clientFd != -1);

            }
            else
            {
                // Check whether client has made a new request, and handle appropriately.
                // If not, check whether client has timed out
                persistence = true;
                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                setsockopt(fds[x].fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                do
                {
                    memset(message, 0, sizeof(message));

                    ssize_t n = recv(fds[x].fd, message, sizeof(message) - 1, 0);

                    // Failed to receive from socket
                    if (n <= 0)
                    {
                        time_t now;
                        time(&now);
                        double difference = difftime(now, fdTimes[x]);
                        if (difference >= 30)
                        {
                            // Timed out
                            persistence = false;
                        }
                        break;
                    }

                    // If we received a request, reset timer and handle the message
                    time(&fdTimes[x]);

                    // Add a null-terminator to the message(request from client)
                    message[n] = '\0';

                    // Count the number of lines in the stringg_str
                    // https://stackoverflow.com/questions/9052490/find-the-count-of-substring-in-string
                    const char *tmp = message;
                    int countLines = 0;
                    // "\r\n" is the newline character
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
                    // Empty unless it's a post request
                    gchar **headerDataSplit = g_strsplit(message, "\r\n\r\n", countLines);
                    gchar *data = headerDataSplit[1];


                    persistence = checkPersistence(headerDataSplit[0], protocol);
                    doMethod(fds[x].fd, methodType, protocol, (struct sockaddr_in *)&client, page, portNoString, data, client);
                    g_strfreev(splitString);
                    g_strfreev(firstLine);
                } while (true);

                // If the connection was not keep-alive, or the connection timed out
                if (!persistence)
                {
                    shutdown(fds[x].fd, SHUT_RDWR);
                    close(fds[x].fd);
                    fds[x].fd = -1;
                    fdRemoved = true;
                }
            }
        }
        if (fdRemoved)
        {
            // Optimize the array
            for (i = 0; i < nfds; i++)
            {
                if (fds[i].fd == -1)
                {
                    for (j = i; j < nfds; j++)
                    {
                        fds[j].fd = fds[j + 1].fd;
                        fdTimes[j] = fdTimes[j + 1];
                    }
                    nfds--;
                }
            }
            fdRemoved = false;
        }
    }
    // If unexpected failure, close the connection
    printf("Server encountered an error, Shutting Down\n");
    shutdown(clientFd, SHUT_RDWR);
    for (i = 0; i < nfds; i++)
    {
        if (fds[i].fd >= 0)
            close(fds[i].fd);
    }
    return 0;
}

// Compares the method value of the header, if the given value matches a supported method the matched method is initiated.
// Handles error if given method is not supported
void doMethod(int clientFd, gchar *methodType, gchar *protocol,
              struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, struct sockaddr_in client)
{
    // Do method depending on the mode
    if (g_strcmp0(methodType, "GET") == 0)
    {
        // Do get
        doGet(clientFd, protocol, clientAddress, page, portNo, client);
    }
    else if (g_strcmp0(methodType, "POST") == 0)
    {
        // Do Post
        doPost(clientFd, protocol, clientAddress, page, portNo, data, client);
    }
    else if (g_strcmp0(methodType, "HEAD") == 0)
    {
        // Do Head
        doHead(clientFd, protocol, clientAddress, page, portNo, client);
    }
    else
    {
        // If we receive an unsupported request type, return an error
        handleUnsupported(clientFd, methodType, protocol, clientAddress, portNo, client);
    }
}

void doGet(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, struct sockaddr_in client)
{
    // The HTML for the page
    char html[DATA_SIZE];
    makeBody(clientAddress, page, portNo, html, NULL, client);

    // Create the header
    char header[HEADER_SIZE];
    makeHeader(protocol, header, strlen(html));

    char response[MESSAGE_SIZE];
    memset(response, 0, sizeof(response));

    strcat(response, header);
    strcat(response, html);

    fprintf(stdout, "\nResponse: \n%s\n", response);
    fflush(stdout);
    // Send data back to client
    send(clientFd, response, strlen(response), 0);
    makeLogFile(clientAddress, "GET", portNo, "200 OK", client);
}

void doPost(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, gchar *data, struct sockaddr_in client)
{
    // The HTML for the page
    char html[DATA_SIZE];
    makeBody(clientAddress, page, portNo, html, data, client);

    // Create the header
    char header[HEADER_SIZE];
    makeHeader(protocol, header, strlen(html));

    char response[MESSAGE_SIZE];
    memset(response, 0, sizeof(response));

    strcat(response, header);
    strcat(response, html);

    fprintf(stdout, "\nResponse: \n%s\n", response);
    fflush(stdout);
    // Send data back to client
    send(clientFd, response, strlen(response), 0);
    makeLogFile(clientAddress, "POST", portNo, "200 OK", client);
}

void doHead(int clientFd, gchar *protocol, struct sockaddr_in *clientAddress, gchar *page, char *portNo, struct sockaddr_in client)
{
    char html[DATA_SIZE];
    makeBody(clientAddress, page, portNo, html, NULL, client);
    // Create the header
    char header[HEADER_SIZE];
    makeHeader(protocol, header, strlen(html));

    fprintf(stdout, "\nHeader Response: \n%s\n", header);
    fflush(stdout);
    // Send data back to client
    send(clientFd, header, strlen(header), 0);
    makeLogFile(clientAddress, "HEAD", portNo, "200 OK", client);
}

// Creates an error response sent to the client if the request he sends contains
// a method that is not supported by the server. Response code 405 for Method Not Allowed
void handleUnsupported(int clientFd, gchar *methodType, gchar *protocol, struct sockaddr_in *clientAddress, char *portNo, struct sockaddr_in client)
{
    // Create the header
    char header[HEADER_SIZE];

    memset(header, 0, HEADER_SIZE * sizeof(char));
    // Fill header with values
    strcat(header, protocol);
    strcat(header, " 405 Method Not Allowed\r\n");
    strcat(header, "Request Method: ");
    strcat(header, methodType);
    strcat(header, "\r\n");
    fprintf(stdout, "\nError Response: \n%s\n", header);
    fflush(stdout);
    send(clientFd, header, strlen(header), 0);
    makeLogFile(clientAddress, methodType, portNo, "405 Method Not Allowed", client);
    //Hotfix for client not disconnection when error
    close(clientFd);
}

// Creates the header
void makeHeader(gchar *protocol, char *header, size_t contentLen)
{
    char *timeStamp = getTime();

    // https://stackoverflow.com/questions/20685080/convert-size-t-to-string
    char contentLenStr[sizeof(contentLen)];
    snprintf(contentLenStr, sizeof(contentLen), "%zu", contentLen);

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

    // Do indicate that the header field is done
    strcat(header, "\r\n");

    free(timeStamp);
}

// Creates the body of the HTML data for the requested page.
void makeBody(struct sockaddr_in *clientAddress, gchar *page, char *portNo, char *html, gchar *data, struct sockaddr_in client)
{
    memset(html, 0, DATA_SIZE * sizeof(char));

    // Open of HTML
    strcat(html, "<!DOCTYPE html><html><body><h1>");

    // Add this to the html URL/RequestedPage ClientIP:Portnumber
    char content[DATA_SIZE];
    memset(content, 0, sizeof(content));
    strcat(content, HOST_URL);
    strcat(content, ":");
    strcat(content, portNo);
    strcat(content, page);
    strcat(content, " ");
    char *clientIP = getIPAddress(clientAddress);
    strcat(content, clientIP);
    strcat(content, ":");
    char *clientPort = g_strdup_printf("%i", client.sin_port);
    strcat(content, clientPort);
    g_free(clientPort);

    // Add content to HTML
    strcat(html, content);
    if (data != NULL)
    {
        strcat(html, data);
    }

    // Close HTML
    strcat(html, "</h1></body></html>\n");
}

// Creates a log file called "httpd.log".  If the file does not exist the program creates it.
// For each request print a single line to a log file that conforms to the format:
// timeStamp : <client ip>:<client port> <request method> <requested URL> : <response code>
void makeLogFile(struct sockaddr_in *clientAddress, gchar *methodType, char *portNo, char *statusCode, struct sockaddr_in client)
{
    FILE *logFile;
    char *clientIP = getIPAddress(clientAddress);

    // Logger
    logFile = fopen("httpd.log", "a");
    if (logFile == NULL)
    {
        printf("Failed to create logfile %s\n", strerror(errno));
    }

    // Get current time
    char *timeStamp = getTime();

    char IPClientStr[INET_ADDRSTRLEN];
    memset(IPClientStr, 0, sizeof(IPClientStr));

    strcpy(IPClientStr, clientIP);

    // Add text to logfile
    // Print time
    fprintf(logFile, "%s : ", timeStamp);
    // Print IP of client
    fprintf(logFile, "%s:", IPClientStr);
    // Print port number
    gchar *clientPort = g_strdup_printf("%i", client.sin_port);
    fprintf(logFile, "%s ", clientPort);
    g_free(clientPort);
    // Print request method
    fprintf(logFile, "%s ", methodType);
    // Print requested URL
    fprintf(logFile, "%s:%s : ", HOST_URL, portNo);
    // Print response code
    fprintf(logFile, "%s\n", statusCode);
    // Close the log file
    fclose(logFile);
    free(timeStamp);
}

// Returns the IP address of the client issuing a request.
char *getIPAddress(struct sockaddr_in *clientAddress)
{
    // https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
    // Get the ip address of a client
    struct in_addr ipAddr = clientAddress->sin_addr;
    char *IPClientStr = malloc(INET_ADDRSTRLEN * sizeof(char));
    inet_ntop(AF_INET, &ipAddr, IPClientStr, INET_ADDRSTRLEN);

    return IPClientStr;
}

// Returns a string with the current date and time.
char *getTime()
{
    // https://stackoverflow.com/questions/1442116/how-to-get-date-and-time-value-in-c-program/30759067#30759067
    // Configure time to the current time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char *timeStamp = malloc(DATE_TIME_SIZE * sizeof(char));
    //DEALLOCATE
    strftime(timeStamp, DATE_TIME_SIZE, "%c", tm);
    //printf("*_*_*_*__*\n%s\n", timeStamp);
    return timeStamp;
}

// Returns a boolean value indicating if the connection requests a persistent connection or not
// True if presistent, false if not.
bool checkPersistence(gchar *header, char *protocol)
{
    //Protocol is of type HTTP/1.1, persistent by default
    if (g_strcmp0(protocol, "HTTP/1.1") == 0 && strstr(header, "Connection: closed") == NULL)
    {
        printf("____Persistent Cause HTTP/1.1\n");
        return true;
    }
    // Check if request specifies persistent connection
    if (strstr(header, "Connection: keep-alive") != NULL)
    {
        printf("____Persistent Cause Connection Keep alive\n");
        return true;
    }
    printf("____NOT persistent\n");
    // Connection is not persistent.
    return false;
}