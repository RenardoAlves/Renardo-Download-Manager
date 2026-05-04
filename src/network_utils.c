#include "network_utils.h"
#include <stdio.h>

#ifndef _WIN32
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

int net_init() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return 1;
#endif
}

void net_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

socket_t net_create_server(int port) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        net_close(sock);
        return INVALID_SOCKET;
    }

    if (listen(sock, 5) == SOCKET_ERROR) {
        net_close(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

socket_t net_accept(socket_t server_sock) {
    struct sockaddr_in addr;
    int len = sizeof(addr);
    return accept(server_sock, (struct sockaddr *)&addr, &len);
}

socket_t net_connect(const char *host, int port) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        net_close(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

int net_send(socket_t sock, const void *data, size_t size) {
    return send(sock, (const char*)data, (int)size, 0);
}

int net_recv(socket_t sock, void *data, size_t size) {
    return recv(sock, (char*)data, (int)size, 0);
}

void net_close(socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}
