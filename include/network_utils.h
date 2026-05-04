#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
#else
    #define socket_t int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#include <stddef.h>

// Inicializa a rede (necessário no Windows)
int net_init();

// Finaliza a rede
void net_cleanup();

// Cria um socket de servidor ouvindo em uma porta
socket_t net_create_server(int port);

// Aceita uma nova conexão de cliente
socket_t net_accept(socket_t server_sock);

// Conecta a um servidor
socket_t net_connect(const char *host, int port);

// Envia dados
int net_send(socket_t sock, const void *data, size_t size);

// Recebe dados
int net_recv(socket_t sock, void *data, size_t size);

// Fecha um socket
void net_close(socket_t sock);

#endif
