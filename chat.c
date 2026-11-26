#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <err.h>
#include <string.h>
#include <wctype.h>

char *trim_right(char*);

int start_server(int);
int accept_connection(int);

int start_client(char*, int);

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

    int peer_fd;

    if (input_buf[0] == 's' || input_buf[0] == 'S') {
        printf("Port number: ");
        fgets(input_buf, sizeof(input_buf), stdin);
        trim_right(input_buf);
        int port_num = atoi(input_buf);

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

        printf("Port Number: ");
        fgets(input_buf, sizeof(input_buf), stdin);
        trim_right(input_buf);
        int port_num = atoi(input_buf);

        peer_fd = start_client(server_ip, port_num);
        printf("Connected to server!\n");
    }

    char hostname[128];
    gethostname(hostname, sizeof(hostname));

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
