#include "utils.h" // Utility definitions
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

void send_data(command_request_t *request, int sock_fd)
{

    if (send(sock_fd, request, sizeof(command_request_t), PF_UNSPEC) == -1)
    {
        perror("Sending request");
    }
}

command_request_t receive_data(int sock_fd)
{
    command_request_t response;
    if (recv(sock_fd, &response, sizeof(command_request_t), PF_UNSPEC) == -1)
    {
        perror("Receiving response");
    }

    return response;
}

command_request_t request_data(command_request_t *request, int sock_fd)
{
    send_data(request, sock_fd);
    return receive_data(sock_fd);
}

void close_socket(int sock_fd)
{
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
}
