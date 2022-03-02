#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <openssl/err.h>

#include "client_comm.h"
#include "client_net.h"

#include "../util.h"
#include "../cmd.h"



int connect_server(char* hostname, int port)
{
    struct hostent *host;
    struct sockaddr_in addr;
 
    if ((host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    
    int sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}



SSL* establish_ssl(SSL_CTX* ctx, int socket)
{
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socket); 

    SSL_set_connect_state(ssl);
    
    int err = SSL_connect(ssl);
    if ( err == -1 )
    {    
        ERR_print_errors_fp(stderr);
 
    }
    return ssl;
}



int execute_client_manager(Connection* conn, pthread_mutex_t* proc_mutex, pthread_mutex_t* message_mutex)
{
    output_message(MSG_INFO, conn, message_mutex, "Using method: %s\n", SSL_get_cipher(get_conn_ssl_session(conn)));

    char buf[MAX_LENGTH_STR_MESSAGE] = {0};

    // 검증 핸드쉐이크을 수행합니다.
    Message* msg = packing_message_create(0, strlen("CLIENT_VERSION 1.0.0"), "CLIENT_VERSION 1.0.0");
    int err = packing_message_send(conn, msg);

    if(err < 0) 
    {
        output_message(MSG_ERROR, conn, message_mutex, "Failed to send the handshake message!\n");
        return -1;
    }

    int bytes = SSL_read(get_conn_ssl_session(conn), buf, sizeof(buf));
    int status_code = response_code_convert(buf);

    free(msg);

    // 핸드쉐이크 실패
    if(status_code == RESPONSE_CONN_SKIP) {
        Message* m = packing_message_convert(buf, bytes);
        output_message(MSG_MESSAGE, conn, message_mutex, "%s\n", packing_message_string(m));
        free(msg);
        free(m);
        return -1;
    }
    else if(status_code == RESPONSE_CONN_FAIL) {
        output_message(MSG_MESSAGE, conn, message_mutex, "%s\n", "Validation Failed.");
        return -1;
    }
    else {
        output_message(MSG_INFO, conn, message_mutex, "서버 검증 응답: %s\n", buf);
    }

    while (1)
    {
        memset(buf, 0, MAX_LENGTH_STR_MESSAGE);
        printf("명령 혹은 채팅 입력: ");
        
        fgets(buf, MAX_LENGTH_STR_MESSAGE, stdin);
        buf[strlen(buf) - 1] = '\0';

        if(strlen(buf) == 0) continue;

        // send message
        msg = msg_packing_message(buf);
        err = packing_message_send(conn, msg);
        
        free(msg);

        if(err < 0)
        {
            output_message(MSG_ERROR, conn, message_mutex, "Failed to send the handshake message!\n");
            return -1;
        }

        memset(buf, 0, MAX_LENGTH_STR_MESSAGE);
        // receive status code
        bytes = SSL_read(get_conn_ssl_session(conn), buf, sizeof(buf));
        if(bytes <= 0)
        {
            output_message(MSG_ERROR, conn, message_mutex, "Failed to receive the handshake message!\n");
            return -1;
        }

        int status_code = response_code_convert(buf);

        if(status_code == RESPONSE_CONN_BYE) {
            break;
        }

        else if(status_code == RESPONSE_CONN_NO_SUPPORT) {
            output_message(MSG_ERROR, conn, message_mutex, "올바른 명령어가 아닙니다.\n");
        }

        else if(status_code == RESPONSE_CONN_FAIL) {
            output_message(MSG_ERROR, conn, message_mutex, "명령을 서버에서 처리하지 못했습니다.\n");
        }

        else if(status_code == RESPONSE_CONN_OK) {
            output_message(MSG_INFO, conn, message_mutex, "명령을 정상적으로 수행하였습니다.\n");
        }

        else if(status_code == RESPONSE_CONN_SKIP) {
            Message* message = packing_message_convert(buf, bytes);

            if(message == (Message*)-1) {
                output_message(MSG_MESSAGE, conn, message_mutex, "서버로부터 메시지를 받았지만, 잘못된 데이터를 수신받았습니다.\n");
            }
            else {

                // 명령 별로 메시지를 처리합니다.
                if(packing_message_command_type(message) == CMD_STATUS) {
                    char *str = packing_message_string(message);
                    char *ptr = strtok(str, "\n");

                    while (ptr != NULL)
                    {
                        output_message(MSG_MESSAGE, conn, message_mutex, "%s\n", ptr);
                        ptr = strtok(NULL, "\n");
                    }
                }

                else if(packing_message_command_type(message) == CMD_SECRET_MESSAGE) {
                    char *str = packing_message_string(message);
                    output_message(MSG_MESSAGE, conn, message_mutex, "%s\n", str);
                }

                else
                {
                    output_message(MSG_MESSAGE, conn, message_mutex, "응답 메시지: %s\n", packing_message_string(message));
                }
                packing_message_free(message);
            }
        }
    }
    output_message(MSG_INFO, conn, message_mutex, "서버에서 연결을 끊었습니다.\n");
    return 0;
}