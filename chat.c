#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>


int start_server(int);
int accept_connection(int);

int start_client(char*, int);

void *recv_task(void *arg);
void *send_task(void *arg);

char *trim_right(char*);

char hostname[128];

pthread_t recv_thread;
pthread_t send_thread;

pthread_mutex_t fd_mutex;
pthread_mutex_t term_mutex;

int peer_fd;

bool terminate;

const char *DEFAULT_IP = "192.168.1.102";
const int DEFAULT_PORT = 8080;

int main() {
    printf("Start a server or client?\n");
    printf("[s]erver/[c]lient: ");
    
    char input_buf[128];
    fgets(input_buf, sizeof(input_buf), stdin);
    trim_right(input_buf);
    if (strlen(input_buf) < 1) {
        fprintf(stderr, "Please select either server or client.\n");
        exit(1);
    }


    if (input_buf[0] == 's' || input_buf[0] == 'S') {
        printf("Port number: ");
        fgets(input_buf, sizeof(input_buf), stdin);
        trim_right(input_buf);
        int port_num;
        if (strlen(input_buf) == 0)
            port_num = DEFAULT_PORT;
        else
            port_num = atoi(input_buf);

        int server_fd = start_server(port_num);
        printf("Server is now listening on port %d\n", port_num);
        peer_fd = accept_connection(server_fd);
        close(server_fd);
        printf("Client connected!\n");
    } else if (input_buf[0] == 'c' || input_buf[0] == 'C') {
        char server_ip[128];
        printf("Server IP Address: ");
        fgets(server_ip, sizeof(server_ip), stdin);
        trim_right(server_ip);
        if (strlen(server_ip) == 0)
            strncpy(server_ip, DEFAULT_IP, sizeof(server_ip));

        printf("Port Number: ");
        fgets(input_buf, sizeof(input_buf), stdin);
        trim_right(input_buf);
        int port_num;
        if (strlen(input_buf) == 0)
            port_num = DEFAULT_PORT;
        else
            port_num = atoi(input_buf);

        peer_fd = start_client(server_ip, port_num);
        printf("Connected to server!\n");
    }

    pthread_mutex_init(&fd_mutex, NULL);
    pthread_mutex_init(&term_mutex, NULL);

    gethostname(hostname, sizeof(hostname));
    hostname[sizeof(hostname) - 1] = '\0';

    terminate = false;

    pthread_create(&recv_thread, NULL, recv_task, NULL);
    pthread_create(&send_thread, NULL, send_task, NULL);

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(peer_fd);

    return 0;
}

int start_server(int port_num) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
        err(1, "Failed to create socket\n");

    struct sockaddr_in socket_addr;
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(port_num);
    socket_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) == -1)
        err(1, "Failed to bind socket to address");

    if (listen(server_fd, 1) == -1)
        err(1, "Failed to listen to socket");

    return server_fd;
}

int accept_connection(int server_fd) {
    int peer_fd = accept(server_fd, NULL, NULL); 
    if (peer_fd == -1)
        err(1, "Failed to accept connection");

    return peer_fd;
}

int start_client(char *server_ip, int port_num) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        err(1, "Failed to create socket");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        err(1, "Failed to connect to server at %s:%d", server_ip, port_num);

    return client_fd;
}

void *recv_task(void *arg) {
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    struct pollfd fd = {
        .fd = peer_fd,
        .events = POLLIN
    };
    while (!terminate && poll(&fd, 1, 100) != -1) {
        if ((fd.revents & POLLIN) > 0) {
            memset(buf, 0, strlen(buf));

            pthread_mutex_lock(&fd_mutex);
            if (recv(peer_fd, buf, sizeof(buf), 0) == 0) {
                pthread_mutex_unlock(&fd_mutex);
                terminate = true;
                break;
            }
            pthread_mutex_unlock(&fd_mutex);

            pthread_mutex_lock(&term_mutex);
            printf("%s\n", buf);
            pthread_mutex_unlock(&term_mutex);
        }
        if ((fd.revents & POLLHUP) > 0) {
            printf("Peer terminated connection\n");
            terminate = true;
        }
        if ((fd.revents & POLLERR) > 0) {
            fprintf(stderr, "Poll resulted in an error\n");
            terminate = true;
        }
        if ((fd.revents & POLLNVAL) > 0) {
            fprintf(stderr, "Poll returned \"Invalid request\"\n");
            terminate = true;
        }
    }
    return NULL;
}

void *send_task(void *arg) {
    struct pollfd fd = {
        .fd = STDIN_FILENO,
        .events = POLLIN,
    };
    char buf[1024];
    while (!terminate && poll(&fd, 1, 100) != -1) {
        if ((fd.revents & POLLIN) > 0) {
            pthread_mutex_lock(&term_mutex);
            fgets(buf, sizeof(buf), stdin);
            pthread_mutex_unlock(&term_mutex);
            trim_right(buf);
            if (strcmp(buf, "/exit") == 0) {
                terminate = true;
                return NULL;
            }

            char *message = malloc(strlen(hostname) + 2 + strlen(buf) + 1);
            strcpy(message, hostname);
            strcat(message, ": ");
            strcat(message, buf);

            pthread_mutex_lock(&fd_mutex);
            send(peer_fd, message, strlen(message), 0);
            pthread_mutex_unlock(&fd_mutex);

            free(message);
        }
        if ((fd.revents & POLLHUP | fd.revents & POLLERR | fd.revents & POLLNVAL) > 0) {
            fprintf(stderr, "stdin poll failed\n");
            terminate = true;
        }
    }
    return NULL;
}

char *trim_right(char *s) {
    int len = strlen(s);
    for (int i = len - 1; i >= 0; i--) {
        if (!iswspace(s[i]))
            break;
        else
            s[i] = '\0';
    }
    return s;
}
