#include <stdio.h>
#include <unistd.h>

#include "../cmd.h"
#include "../connection.h"
#include "../comm.h"
#include "../util.h"

char* random_binary_data(int length)
{
    char charset[0x100] = {0};
    char randomString[length];

    for(int i = 0 ; i < 0xFF; i++)
    {
        strcat(charset, (char)i);
    }


    for (int n = 0; n < length; n++) {            
        int key = rand() % (int)(sizeof(charset) -1);
        randomString[n] = charset[key];
    }
    randomString[length] = '\0';
    
    return randomString;
}



int main(int argc, char* argv[])
{
    SSLContext* ctx = create_client_ssl_context(true);
    Connection* conn = connection_create(ctx);

    pthread_mutex_t message_mutex, socket_mutex;
    
    int socket_id = connect_server("localhost", "4433");
    if(socket_id == -1) {
        perror("Can\'t established the server!");
        abort();
    }

    SSL* ssl = establish_ssl(get_ssl_ctx_context(ctx), socket_id);
    set_conn_ssl_session(conn, ssl);

    pthread_mutex_init(&message_mutex, NULL);
    pthread_mutex_init(&socket_mutex, NULL);

    Message* msg = packing_message_create(0, strlen("CLIENT_VERSION 1.0.0"), "CLIENT_VERSION 1.0.0");
    int err = packing_message_send(conn, msg);

    if(err < 0) 
    {
        output_message(MSG_ERROR, conn, &message_mutex, "Failed to send the handshake message!\n");
        return -1;
    }

    char buf[MAX_LENGTH_MESSAGE];

    int bytes = SSL_read(get_conn_ssl_session(conn), buf, sizeof(buf));
    int status_code = response_code_convert(buf);
    free(msg);

    if(status_code == RESPONSE_CONN_WELCOME)
    {
        char* rstr = random_binary_data(255);
        Message* msg = packing_message_create(CMD_BROADCASE_MESSAGE, 255, rstr);
        output_message(MSG_MESSAGE, conn, &message_mutex, "Sending the ranom message\n");
        err = packing_message_send(conn, msg);
        bytes = SSL_read(get_conn_ssl_session(conn), buf, sizeof(buf));
        sleep(1);
        free(msg);
    }

    else
    {
        output_message(MSG_ERROR, conn, &message_mutex, "Failed to verifty the client!\n");
    }

    disconnect(conn, &socket_mutex, true);
    SSL_CTX_free(get_ssl_ctx_context(ctx));
    free(ctx);
    free(conn);
    return 0;
}