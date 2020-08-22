// Make connection to server
// Recieve a client id from the server
// On a loop, check for incomming commands through stdin
// If command is entered, send to server, and recieve information based upon that command

#include "utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 80

char *SERVER;
int PORT;

int sock_fd = 0;
char socket_buffer[BUFFER_SIZE]; // Buffer for incoming/outgoing information from/to the server
char stdin_buffer[BUFFER_SIZE];  // Buffer for reading commands from stdin

void create_server_connection(char *argv[]);
void close_down_client();
void verify_session();
void command_input();

int main(int argc, char **argv)
{
    signal(SIGINT, close_down_client);
    signal(SIGHUP, close_down_client);

    // Read in server IP address and port number first, argv, 1 & 2
    if (argc == 1)
    {
        fprintf(stderr, "Usage: ./client [server ip address] [port number]");
        exit(1);
    }

    create_server_connection(argv);
    command_input();

    return 0;
}

/***********************************************************************
 * func:            A function used to form a basic connection 
 *                  to the server.
***********************************************************************/
void create_server_connection(char *argv[])
{
    // Variables related to client-server communication
    struct sockaddr_in serverAddr;
    struct hostent *host;

    SERVER = argv[1];     // First cli argument (Server IP)
    PORT = atoi(argv[2]); // Second cli argument (Port Number)

    if ((host = gethostbyname(argv[1])) == NULL) /* get the host info */
    {
        herror("Could not get host by name");
        exit(1);
    }

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) == -1) // Create the socket
    {
        perror("Failed to create socket");
        exit(1);
    }

    printf("Successfully created client socket\n");

    serverAddr.sin_family = AF_INET;   /* host byte order */
    serverAddr.sin_port = htons(PORT); /* short, network byte order */
    serverAddr.sin_addr = *(struct in_addr *)host->h_addr;
    bzero(&serverAddr.sin_zero, 8); /* zero the rest of the struct */

    // Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) == -1) // Attempt to connect to the server
    {
        perror("Failed to connect to server");
        exit(1); // Exit with failure code
    }

    printf("Successfully connected to the server, please enter commands below:\n");
}

/***********************************************************************
 * func:            A function used to read input from the terminal 
 *                  and perform requests to the server, depending on
 *                  the command 
***********************************************************************/
void command_input()
{

    // Take input from the terminal forever
    while (*fgets(stdin_buffer, 20, stdin) != '\0')
    {
        // Make sure buffer input is at least 4 characters
        if (strlen(stdin_buffer) < 4)
            continue;
        else
            stdin_buffer[strlen(stdin_buffer) - 1] = '\0'; // Make sure buffer ends in EOF char, so sscanf can read string

        //  Declare a variable called command to hold command string
        char command[10];
        char message[100];
        int channel_id;

        //  Use a single call to fscanf to (attempt to) read a char and six int
        //  values into command, and your integer variables. Capture the value
        //  returned by fscanf for later use.
        int captured = sscanf((const char *)&stdin_buffer, "%s %d %[^\t\n]", command, &channel_id, message);

        command_request_t request;

        if (captured == 1) // If command is just one word
        {
            if (strncmp(command, "CHANNELS", 8) == 0)
            {
                request.request_type = CHANNELS;
                request = request_data(&request, sock_fd); // Send CHANNEL request
                printf("%s", request.message);
            }
            else if (strncmp(command, "LIVEFEED", 8) == 0)
            {
                request.request_type = LIVEFEED;
                send_data(&request, sock_fd); // Send LIVEFEED request
            }
            else if (strncmp(command, "NEXT", 4) == 0)
            {
                request.request_type = NEXT;
                request = request_data(&request, sock_fd); // Send NEXT request
                printf("%s", request.message);
            }
            else if (strncmp(command, "BYE", 3) == 0)
            {
                close_down_client();
            }
        }
        else if (captured == 2) // If the command contains a channel_id
        {
            if (strncmp(command, "SUB", 3) == 0)
            {
                request.request_type = SUB;
                request.channel_id = channel_id;
                request = request_data(&request, sock_fd); // Send SUB request
                printf("%s", request.message);
            }
            else if (strncmp(command, "UNSUB", 5) == 0)
            {
                request.request_type = UNSUB;
                request.channel_id = channel_id;
                request = request_data(&request, sock_fd); // Send UNSUB request
                printf("%s", request.message);
            }
            else if (strncmp(command, "LIVEFEED", 8) == 0)
            {
                request.request_type = LIVEFEEDID;
                request.channel_id = channel_id;
                send_data(&request, sock_fd); // Send LIVEFEED request
            }
            else if (strncmp(command, "NEXT", 4) == 0)
            {
                request.request_type = NEXTID;
                request.channel_id = channel_id;
                request = request_data(&request, sock_fd); // Send NEXT request
                printf("%s", request.message);
            }
        }
        else if (captured == 3) // If sending a message
        {
            if (strncmp(command, "SEND", 4) == 0)
            {
                request.request_type = SEND;
                request.channel_id = channel_id;

                memset(request.message, '\0', sizeof(request.message));
                strncpy(request.message, message, strlen(message));

                request = request_data(&request, sock_fd); // Send SEND request
                printf("%s", request.message);
            }
        }

        memset(stdin_buffer, 0, sizeof(stdin_buffer));
        fflush(stdin);
    }
}

/***********************************************************************
 * func:            A function used to gracefully exit the client and
 *                  properly close the connection with the server.
***********************************************************************/
void close_down_client()
{
    if (sock_fd != 0)
    {
        printf("\nThe client is now shutting down");

        // Send request telling server client is quitting, so it can close the currently connected socket
        command_request_t request;
        request.request_type = BYE;
        send_data(&request, sock_fd);

        // Can now shutdown the client
        shutdown(sock_fd, SHUT_RDWR); // Shutdown the current socket
        close(sock_fd);               // Close the socket
        printf("\nSuccessfully closed connection to %s:%d", SERVER, PORT);
    }

    exit(0); // Exit successfully
}
