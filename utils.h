#ifndef UTILS_H_
#define UTILS_H_
#include <stdbool.h>

#define MAX_MESSAGE_SIZE (1024)
#define MAX_CHANNELS (255)
#define MAX_MESSAGES (100)

// Define the different possible command types
typedef enum
{
    SUB,
    CHANNELS,
    UNSUB,
    NEXT,
    NEXTID,
    LIVEFEEDID,
    LIVEFEED,
    SEND,
    BYE,
} request_t;

typedef struct
{
    int client_id;
} client;

// Request which is used to send data to the server
typedef struct
{
    int channel_id;                 // Optional
    char message[MAX_MESSAGE_SIZE]; // Optional
    request_t request_type;         // Required
} command_request_t;

// Message which is stored on the server
typedef struct
{
    bool read;
    char message[MAX_MESSAGE_SIZE];
} message;

// A single channel, contained on the server
typedef struct
{
    bool subscribed;
    int messages_sent;
    int messages_read;
    int messages_unread;
    message messages[MAX_MESSAGES];
} channel;

// A list of channels and the number of subscriptions
typedef struct
{
    int subscriptions;
    channel channel_list[MAX_CHANNELS];
} channel_list_t;
// Function definitions

/***********************************************************************
 * func:            A function used to send requests to the server 
***********************************************************************/
void send_data(command_request_t *request, int sock_fd);

/***********************************************************************
 * func:            A function used to recieve requests from the server 
***********************************************************************/
command_request_t receive_data(int sock_fd);

/***********************************************************************
 * func:            A function used to send requests to the server 
 *                  and return the data requested 
***********************************************************************/
command_request_t request_data(command_request_t *request, int sock_fd);

/***********************************************************************
 * func:            A function used to properly terminate a given
 *                  socket.
 * param socket_fd: The socket_fd of the required socket to be 
 *                  terminated.
***********************************************************************/
void close_socket(int sock_fd);

#endif