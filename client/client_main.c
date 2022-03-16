#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>

#include "client_net.h"
#include "client_comm.h"
#include "../comm.h"
#include "../connection.h"

#define FAIL -1

int main(int argc, char *argv[])
{ 
    pthread_mutex_t message_mutex;
    pthread_mutex_t socket_mutex;

    if (argc < 3)
    {
        printf("usage: %s <hostname> <portnum>\n", argv[0]);
        exit(0);
    }

    char* hostname = argv[1];
    int port = atoi(argv[2]);

    SSLContext* ctx = create_client_ssl_context(true);
    Connection* conn = connection_create(ctx);

    pthread_mutex_init(&message_mutex, NULL);
    pthread_mutex_init(&socket_mutex, NULL);

    int socket_id = connect_server(hostname, port);

    if(socket_id == -1) {

    }

    SSL* ssl = establish_ssl(get_ssl_ctx_context(ctx), socket_id);
    set_conn_ssl_session(conn, ssl);
    set_conn_socket_id(conn, socket_id);


    execute_client_manager(conn, &socket_mutex, &message_mutex);

    disconnect(conn, &socket_mutex, true);

    SSL_CTX_free(get_ssl_ctx_context(ctx));        /* release context */
    free(ctx);
    free(conn);
    return 0;
}