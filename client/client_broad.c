#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "client_net.h"

#include "../comm.h"
#include "../connection.h"
#include "../util.h"
#include "../cmd.h"


int main(int argc, char* argv[])
{
    pthread_mutex_t message_mutex;
    pthread_mutex_t socket_mutex;
    output_message(MSG_CONNECTION, NULL, NULL, "Started message receiver\n");
    if (argc < 2)
    {
        printf("usage: %s <hostname>\n", argv[0]);
        exit(0);
    }

    char* hostname = argv[1];
    int port = 8443;

    SSLContext* ctx = create_client_ssl_context(true);
    Connection* conn = connection_create(ctx);

    pthread_mutex_init(&message_mutex, NULL);
    pthread_mutex_init(&socket_mutex, NULL);

    int socket_id = connect_server(hostname, port);
    if(socket_id == -1) {
        abort();
    }

    SSL* user_session = establish_ssl(get_ssl_ctx_context(ctx), socket_id);
    set_conn_ssl_session(conn, user_session);


    pthread_mutex_init(&message_mutex, NULL);
    pthread_mutex_init(&socket_mutex, NULL);

    bool isSuspend = false;
    output_message(MSG_CONNECTION, NULL, &message_mutex, "Connected to chat server\n");
    
    while (!isSuspend)
    {
        Message* msg = packing_message_create(CMD_BROADCASE_MESSAGE, 0, "");
        packing_message_send(conn, msg);

        free(msg);

        // 메시지 대기
        char buf[MAX_LENGTH_MESSAGE] = {0, };
        int bytes = SSL_read(user_session, buf, MAX_LENGTH_MESSAGE);

        if(bytes <= 0) {
            int error = ERR_get_error();
            output_message(MSG_ERROR, NULL, &message_mutex, "TLS Connection Failed, Reason: %s\n", ERR_error_string(error, NULL));
            output_message(MSG_ERROR, NULL, &message_mutex, "클라이언트와 연결이 종료되어, 서버에서 연결을 끊었습니다\n");
            break;
        }

        msg = packing_message_convert(buf, bytes);

        // 비정상적인 패킷인지 확인힙니다.
        if( __builtin_expect(msg == (Message*)-1, 0)) {
            output_message(MSG_ERROR, NULL, &message_mutex, "Forcibly disconnected, packet validation failed\n");
            break;
        }

        int func_type = packing_message_command_type(msg);

        switch(func_type) {
            case CMD_BROADCASE_MESSAGE:
                output_message(MSG_MESSAGE, NULL, &message_mutex, "%s\n", packing_message_string(msg));
                break;
            case CMD_DISCONNECT:
                output_message(MSG_MESSAGE, NULL, &message_mutex, "Requested disconnected to server");
                free(msg);
                isSuspend = true;
                break;
            default:
                break;
        }
        free(msg);
    }

    SSL_shutdown(user_session);
    SSL_free(user_session);

    close(socket_id);

    SSL_CTX_free(get_ssl_ctx_context(ctx));
    return 0;
}
