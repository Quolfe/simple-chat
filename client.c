#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string.h>
#include <wctype.h>

char *trim_right(char*);

const uint16_t PORT_NUM = 8080;
const char* SERVER_ADDR_STR = "192.168.1.102";

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
        err(1, "Failed to create socket");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NUM);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR_STR);

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        err(1, "Failed to connect to server at %s:%d", SERVER_ADDR_STR, PORT_NUM);

    printf("Connected to server!\n");

    char message_buf[1024];
    while (1) {
        memset(message_buf, 0, sizeof(message_buf));
        printf("Message: ");
        fgets(message_buf, sizeof(message_buf), stdin);
        trim_right(message_buf);
        if (strcmp(message_buf, "/exit") == 0)
            break;
        send(client_fd, message_buf, strlen(message_buf), 0);
    }

    close(client_fd);

    return 0;
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
