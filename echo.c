#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

//---- client handler function ----//
void* handle_client(void* client_socket) {
    int socket = *(int*)client_socket;
    char buffer[100] = {0};
    ssize_t bytes_read;

    while ((bytes_read = read(socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Message: %s", buffer);
        write(socket, buffer, bytes_read); // Sends message back to client
    }

    close(socket);
    free(client_socket);
    return NULL;
}

//---- echo server function ----//
void* echo_server(void* v_port) {
    int port = *(int*)v_port;
    int server_file_desc, *new_socket;
    int opt = 1;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // Create server socket
    server_file_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (server_file_desc < 0) {
        perror("Error creating socket");
        return NULL;
    }

    // Set socket options
    if (setsockopt(server_file_desc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error setting socket options");
        close(server_file_desc);
        return NULL;
    }

    // Bind socket to the specified port
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_file_desc, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error binding socket");
        close(server_file_desc);
        return NULL;
    }

    // Listen for incoming connections
    if (listen(server_file_desc, 3) < 0) {
        perror("Error listening on socket");
        close(server_file_desc);
        return NULL;
    }

    printf("Echo server listening on port %d\n", port);

    // Accept and handle connections in a loop
    while (1) {
        new_socket = malloc(sizeof(int));
        *new_socket = accept(server_file_desc, (struct sockaddr*)&addr, &addrlen);
        if (*new_socket < 0) {
            perror("Error accepting connection");
            free(new_socket);
            continue;
        }

        // Create a new thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, new_socket) != 0) {
            perror("Error creating thread");
            close(*new_socket);
            free(new_socket);
        }

        pthread_detach(tid);
    }

    close(server_file_desc);
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    pthread_t server_thread;

    if (pthread_create(&server_thread, NULL, echo_server, &port) != 0) {
        perror("Error creating server thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(server_thread, NULL);

    return 0;
}
