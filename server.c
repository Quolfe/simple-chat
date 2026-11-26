#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <err.h>
#include <string.h>

const uint16_t PORT_NUM = 8080;

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
        err(1, "Failed to create socket\n");

    struct sockaddr_in socket_addr;
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(PORT_NUM);
    socket_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) == -1)
        err(1, "Failed to bind socket to address");

    if (listen(server_fd, 1) == -1)
        err(1, "Failed to listen to socket");

    printf("Server is now listening on port %d\n", PORT_NUM);

    struct sockaddr_in peer_socket;
    socklen_t sockaddr_len = sizeof(peer_socket);

    int peer_fd = accept(server_fd, (struct sockaddr*)&peer_socket, &sockaddr_len); 
    if (peer_fd == -1)
        err(1, "Failed to accept connection");

    printf("Client connected!\n");

    close(server_fd);

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    int res;
    while ((res = recv(peer_fd, buf, sizeof(buf), 0)) != -1 && res != 0) {
        printf("%s\n", buf);
        memset(buf, 0, sizeof(buf));
    }

    close(peer_fd);

    return 0;
}
