#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

#define DEFAULT_PORT 5000
#define MAX_CLIENTS 10
#define BUFFER_SIZE 512
#define USERNAME_SIZE 64

typedef struct {
    int socket;
    char *username;
} Client;

int main(void)
{
    printf("Server running on port %d...\n", DEFAULT_PORT);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(DEFAULT_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 5) == -1)
    {
        perror("listen");
        close(server_socket);
        return 1;
    }

    Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        Client client = {-1, NULL};
        clients[i] = client;
    }

    fd_set read_fds;
    int max_sd;

    int running = 1;
    while (running)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        FD_SET(0, &read_fds); // monitor stdin
        max_sd = server_socket;

        // add client sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].socket > 0)
            {
                FD_SET(clients[i].socket, &read_fds);
                if (clients[i].socket > max_sd)
                    max_sd = clients[i].socket;
            }
        }

        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR)
        {
            perror("select");
        }

        // check stdin for quit command
        if (FD_ISSET(0, &read_fds))
        {
            char cmd[16];
            if (fgets(cmd, sizeof(cmd), stdin))
            {
                if (cmd[0] == 'q') // quit on 'q' + Enter
                {
                    printf("Shutting down server...\n");
                    running = 0;
                    break;
                }
            }
        }

        // incoming connection
        if (FD_ISSET(server_socket, &read_fds))
        {
            int new_socket = accept(server_socket, NULL, NULL);
            if (new_socket < 0)
            {
                perror("accept");
                continue;
            }

            printf("New client connected: %d\n", new_socket);

            // add to clients list
            char uname[USERNAME_SIZE];
            int uname_bytes = recv(new_socket, uname, sizeof(uname)-1, 0);
            if (uname_bytes > 0)
            {
                uname[uname_bytes] = '\0';
            }

            // store username in the clients array
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket == -1)
                {
                    clients[i].socket = new_socket;
                    clients[i].username = strdup(uname); // dynamically store it
                    added = 1;
                    break;
                }
            }
            if (!added)
            {
                printf("Max clients reached, rejecting connection\n");
                close(new_socket);
            }
        }


        char buffer[BUFFER_SIZE];

        // check all clients for incoming messages
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sd = clients[i].socket;
            char *username = clients[i].username;
            if (sd > 0 && FD_ISSET(sd, &read_fds))
            {
                int bytes = recv(sd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0)
                {
                    // client disconnected
                    printf("Client disconnected: %s\n", clients[i].username);
                    free(clients[i].username);
                    close(sd);
                    clients[i].username = NULL;
                    clients[i].socket = -1;
                }
                else
                {
                    buffer[bytes] = '\0';
                    printf("Received from client %s: %s\n", username, buffer);

                    char sendbuf[BUFFER_SIZE + USERNAME_SIZE];
                    snprintf(sendbuf, sizeof(sendbuf), "%s: %s", clients[i].username, buffer);

                    // broadcast to all other clients
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].socket != -1 && clients[j].socket != sd)
                        {
                            send(clients[j].socket, sendbuf, strlen(sendbuf), 0);
                        }
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
