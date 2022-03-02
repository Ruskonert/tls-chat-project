#ifndef CLIENT_NET_H
#define CLIENT_NET_H

#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>

#include <openssl/ssl.h>

#include "../connection.h"

struct client_t;
typedef struct client_t Client;

int connect_server(char* hostname, int port);

SSL* establish_ssl(SSL_CTX* ctx, int socket);

int execute_client_manager(Connection* conn, pthread_mutex_t* proc_mutex, pthread_mutex_t* message_mutex);

#endif