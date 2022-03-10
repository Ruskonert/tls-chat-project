/**
 * @file server_comm.c
 * @author ruskonert
 * @brief 클라이언트와 서버 프로그램 간 패킷을 읽어들이고, 메시지로 변환 및 
 *        처리하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-18
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server_main.h"
#include "server_net.h"
#include "server_broad.h"

#include "../connection.h"
#include "../comm.h"
#include "../util.h"


/**
 * @brief Packing Message를 읽어들이고, 그에 따른 명령어를 실행합니다.
 * 
 * @param ctx 명령을 요청한 유저 컨텍스트가 포함됩니다.
 * @param message Packing Message가 포함됩니다.
 * @return int 명령어 실행에 대한 response code 값이 반환됩니다.
 */
int execute_command(User* ctx, Message* message) {
    if(packing_message_command_type(message) >= MAX_LENGTH_COMMAND) return RESPONSE_CONN_NO_SUPPORT;
    COMMAND_FUNC func = get_prog_command_function()[packing_message_command_type(message)];

    if(func == 0) return RESPONSE_CONN_NO_SUPPORT;

    return func(ctx, message);
}

/**
 * @brief 유저와 서버 간 패킷을 송수신합니다. 이 작업은 비동기 기반으로 수행합니다. 
 * 
 * @param argv 유저 컨텍스트 값이 포함됩니다.
 * @return void* 동작 실패시 -1, 그렇지 않다면 0을 반환합니다.
 */
void* communicate_user(void* argv)
{
    User* ctx = (User*)argv;
    ConnectManager* cm = get_user_connect_manager(ctx);
    pthread_mutex_t* mutex = get_connect_manager_of_mutex(cm);

    Connection* conn = get_user_conn(ctx);
    SSL* user_session = get_conn_ssl_session(conn);

    output_message(MSG_CONNECTION, conn, mutex, "Joined the chat server!\n");

    show_certs(ctx, user_session);

    while(get_conn_status(conn) != USER_STATUS_SUSPEND) {
        char buf[MAX_LENGTH_MESSAGE] = {0, };
        int bytes = SSL_read(user_session, buf, MAX_LENGTH_MESSAGE);

        // TLS 통신 에러 발생시 연결을 강제 종료합니다.
        if(bytes <= 0) {
            int error = ERR_get_error();
            output_message(MSG_ERROR, conn, mutex, "TLS Connection Failed when communicating the user, Reason: %s\n", ERR_error_string(error, NULL));
            set_conn_status(conn, USER_STATUS_SUSPEND);
            break;
        }

        Message* msg = packing_message_convert(buf, bytes);
         
        // 패킷 규격에 맞지 않는 비정상적인 패킷인지 검증합니다.
        if( __builtin_expect(msg == (Message*)-1, 0)) {
            output_message(MSG_CONNECTION, conn, mutex, "Forcibly disconnected, packet validation failed\n");
            set_conn_status(conn, USER_STATUS_SUSPEND);
        }

        else {
            int err = execute_command(ctx, msg);
            send_response_code(err, conn, mutex);
            free(msg);
        }
    }
    
    user_broad_disconnect(ctx);
    user_disconnect(ctx, true);
    return (void*)0;
}