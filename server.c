// TODO: Form basic connection to a client
// TODO: For now just print inoformation sent from the cliest_t test
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 8080
#define BACKLOG 5

channel_list_t channels;
int listen_fd, client_fd;
bool connected = false;

// Function definitions (potential prototypes)
void close_socket(int client_fd);
void close_down_server(int sock_fd);
void initalize_channels();
void start_server();

void handle_incoming_connections();
void process_incoming_commands();

void verify_client();
bool is_valid_channel(int channel_id);

void subscribe_to_channel(int channel_id);
void unsubscribe_from_channel(int channel_id);
void channel_subscription_information();
void add_message_to_channel(int channel_id, char *message);

void get_next_message(int channel_id);
void get_next_messages();

void get_channel_livefeed(int channel_id);
void get_livefeed();

void send_message(const char *format, ...);

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr, client_addr;
    int server_len = sizeof(server_addr);
    int client_len = sizeof(client_addr);

    signal(SIGINT, close_down_server);
    signal(SIGHUP, close_down_server);

    // Decide to use the default port or custom port depending on how many arugments supplied
    switch (argc)
    {
    case 1:
        server_addr.sin_port = htons(DEFAULT_PORT); // Use default port
        break;
    case 2:
        server_addr.sin_port = htons(atoi(argv[1])); // Use custom port
        break;
    default:
        printf("Usage: ./server [port_number] or ./server\n");
        exit(1);
    }

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC)) == -1)
    {
        perror("Creating socket");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Successfully created socket\n");
    }

    server_addr.sin_family = AF_INET;                /* host byte order */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* auto-fill with my IP */

    if (bind(listen_fd, (struct sockaddr *)&server_addr, server_len) == -1)
    {
        perror("Binding socket to server port");
        exit(-1);
    }

    printf("Successfully binded socket to server port\n");

    /* Start listening on socket port */
    if (listen(listen_fd, BACKLOG) == -1)
    {
        perror("Listening on socket");
        exit(1);
    }

    printf("Successfully listening on socket\n");

    for (;;)
    {
        // Handle incoming connections
        if (!connected)
        {
            if ((client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len)) == -1)
            {
                perror("Failed to accept incoming connection");
            }

            printf("Successfully connected to client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            connected = true;
        }
        else
        {
            process_incoming_commands(client_fd);
        }
    }
}

/***********************************************************************
 * func:          Process incoming commands sent from the client 
 * 
 * param client_fd  The file descriptor used to connect the client and 
 *                  server
***********************************************************************/
void process_incoming_commands(int client_fd)
{

    // Recieve incoming commands
    command_request_t res;
    res = receive_data(client_fd);

    // Process commands
    switch (res.request_type)
    {
    case SUB:
        subscribe_to_channel(res.channel_id);
        break;
    case CHANNELS:
        channel_subscription_information();
        break;
    case UNSUB:
        unsubscribe_from_channel(res.channel_id);
        break;
    case NEXTID:
        get_next_message(res.channel_id);
        break;
    case LIVEFEEDID:
        printf("LIVEFEED %d\n", res.channel_id);
        break;
    case NEXT:
        get_next_messages();
        break;
    case LIVEFEED:
        printf("LIVEFEED\n");
        break;
    case SEND:
        add_message_to_channel(res.channel_id, res.message);
        break;
    case BYE:
        close_socket(client_fd);

        // Unsubscribe client from all channels;

        for (size_t i = 0; i < 255; i++)
        {
            channels.channel_list[i].subscribed = false;
        }

        channels.subscriptions = 0;
        connected = false;
        break;
    default:
        printf("Could not read command\n");
    }
}

/***********************************************************************
 * func:           Subscribes the client to a specific channel 
***********************************************************************/
void subscribe_to_channel(int channel_id)
{
    if (!is_valid_channel(channel_id))
    {
        send_message("Invalid channel: %d\n", channel_id);
    }
    else if (channels.channel_list[channel_id].subscribed)
    {
        send_message("Already subscribed to channel %d\n", channel_id);
    }
    else
    {
        channels.channel_list[channel_id].subscribed = true;
        channels.subscriptions++;
        send_message("Subscribed to channel %d\n", channel_id);
    }
}

/***********************************************************************
 * func:           Unsubscribes the client from a specific channel 
***********************************************************************/
void unsubscribe_from_channel(int channel_id)
{

    if (!is_valid_channel(channel_id))
    {
        send_message("Invalid channel: %d\n", channel_id);
    }
    else if (!channels.channel_list[channel_id].subscribed)
    {
        send_message("Not subscribed to channel %d\n", channel_id);
    }
    else
    {
        channels.channel_list[channel_id].subscribed = false;
        channels.subscriptions--;
        send_message("Unsubscribed from channel %d\n", channel_id);
    }
}

/***********************************************************************
 * func:            Sends the information regarding the subscribed channels
 *                  such as sent messages, unread messages and read messages
*                   back to the client.
***********************************************************************/
void channel_subscription_information()
{
    char message[200];
    char current_message_str[30];
    for (size_t i = 0; i < MAX_CHANNELS; i++)
    {
        if (channels.channel_list[i].subscribed)
        {
            snprintf(current_message_str, 30, "Channel %d: %d\t%d\t%d\n", i, channels.channel_list[i].messages_sent, channels.channel_list[i].messages_read, channels.channel_list[i].messages_unread);
            strcat(message, current_message_str);
        }
    }
    send_message(message); // Send channel information to the client
}

/***********************************************************************
 * func:            A function which gets the message sent from the client
 *                  and adds the message to the given channel 
 * 
 * param channel_id: the channel which the message will be queued into
 * param sent_message: the string which will be queued 
***********************************************************************/
void add_message_to_channel(int channel_id, char *sent_message)
{
    if (is_valid_channel(channel_id))
    {
        channels.channel_list[channel_id].messages_sent++;
        channels.channel_list[channel_id].messages_unread++;
        message *current_message;
        for (size_t i = 0; i < MAX_MESSAGES; i++)
        {
            current_message = &channels.channel_list[channel_id].messages[i];
            if (strcmp(current_message->message, "") == 0)
            {
                strcpy(current_message->message, sent_message);
                break;
            }
        }
        send_message("");
    }
    else
    {
        send_message("Invalid channel: %d\n", channel_id);
    }
}

/***********************************************************************
 * func:            A function which sends a the next unread message
 *                  in the given channel
 * 
 * param channel_id: the channel which will be checked for the next
 *                   unread message
***********************************************************************/
void get_next_message(int channel_id)
{
    message *current_message;
    if (is_valid_channel(channel_id))
    {
        for (size_t i = 0; i < MAX_MESSAGES; i++)
        {
            current_message = &channels.channel_list[channel_id].messages[i];
            if (channels.channel_list[channel_id].subscribed)
            {
                if (!current_message->read)
                {
                    send_message("%s\n", current_message->message);
                    current_message->read = true;
                    break;
                }
            }
            else
            {
                send_message("Not subscribed to channel: %d\n", channel_id);
                break;
            }
        }
    }
    else
    {
        send_message("Invalid channel: %d\n", channel_id);
    }
}

/***********************************************************************
 * func:            A function which sends a the next unread messages
 *                  in all of the subscribed channels 
***********************************************************************/
void get_next_messages()
{
    if (channels.subscriptions != 0)
    {
        char messages[200];
        char current_message_str[30];
        message *current_message;
        for (int channel_id = 0; channel_id < MAX_CHANNELS; channel_id++)
        {
            for (size_t i = 0; i < MAX_MESSAGES; i++)
            {
                current_message = &channels.channel_list[channel_id].messages[i];
                if (channels.channel_list[channel_id].subscribed)
                {
                    if (!current_message->read)
                    {
                        snprintf(current_message_str, 30, "%d:%s\n", channel_id, current_message->message);
                        strcat(messages, current_message_str);
                        current_message->read = true;
                        break;
                    }
                }
            }
        }
        send_message("%s", messages);
    }
    else
    {
        send_message("Not subscribed to any channels.\n");
    }
}

/***********************************************************************
 * func:            A function which sends a formatted message to the client 
***********************************************************************/
void send_message(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    command_request_t res;
    vsnprintf(res.message, sizeof(message), format, args);
    send_data(&res, client_fd);
}

/***********************************************************************
 * func:            A function which determines if the given channel_id
 *                  is valid 
 * 
 * param channel_id: The socket_fd of the required to determine if it is
 *                  valid
***********************************************************************/
bool is_valid_channel(int channel_id)
{
    return (channel_id >= 0 && channel_id <= 255);
}

/***********************************************************************
 * func:            A function used to properly shutdown the server 
 * 
 * param sock_fd: The socket_fd of the required socket to be 
 *                  terminated.
***********************************************************************/
void close_down_server(int sock_fd)
{
    printf("\nThe server is now shutting down\n");
    shutdown(listen_fd, SHUT_RDWR); // Send request to client to start shutting
    close(listen_fd);
    printf("The server has successfully closed down");

    exit(0);
}