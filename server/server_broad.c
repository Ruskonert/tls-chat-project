/**
 * @file server_broad.h
 * @author ruskonert
 * @brief 클라이언트의 메시지 리시버를 대상으로 연결을 수행하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-23
 */

#include <signal.h>
#include <netdb.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <openssl/err.h>

#include "server_net.h"
#include "server_broad.h"

#include "../cmd.h"

void* broad_send_message(void* argv)
{
    void** argv_ptr = (void**)argv;

    User* ctx = argv_ptr[0];
    ScheduleMessage* smessage = argv_ptr[1];

    if(ctx == 0) return -1;

    Message* message = schedule_message_origin(smessage);

    ConnectManager* cm = get_user_connect_manager(ctx);
    Connection* broad_conn = get_user_broad_conn(ctx);

    pthread_mutex_t* mutex = get_connect_manager_of_mutex(cm);
    pthread_mutex_t* user_mutex = get_user_mutex(ctx);
    
    pthread_mutex_lock(mutex);

    if(broad_conn == 0) 
    {
        pthread_mutex_unlock(mutex);
        return -1;
    }

    if(get_conn_status(get_user_broad_conn(ctx)) != USER_STATUS_JOINED_AND_WAIT_MESSAGE)
    {
        pthread_mutex_unlock(mutex);
        return -1;
    }

    Connection* user_conn = get_user_conn(ctx);
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(cm);

    // 메시지 전송 수행 이후, 스케줄 메시지의 카운트를 증가시킵니다.
    packing_message_send(broad_conn, message);
    schedule_message_increase(smessage);

    set_conn_status(broad_conn, USER_STATUS_JOINED_SERVER);
    pthread_mutex_unlock(user_mutex);
    pthread_mutex_unlock(mutex);
    return 0;
}



void* communicate_broad_user(void* argv)
{
    User* ctx = (User*)argv;

    ConnectManager* broad_cm = get_user_broad_connect_manager(ctx);

    pthread_mutex_t* mutex = get_connect_manager_of_mutex(broad_cm);
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(broad_cm);
    pthread_mutex_t* user_mutex = get_user_mutex(ctx);

    Connection* conn = get_user_conn(ctx);
    Connection* broad_conn = get_user_broad_conn(ctx);

    SSL* user_broad_session = get_conn_ssl_session(broad_conn);

    output_message(MSG_CONNECTION, conn, message_mutex, "Connected the broadcast channel\n");
    set_conn_status(broad_conn, USER_STATUS_JOINED_SERVER);

    show_certs(ctx, user_broad_session);
    signal(SIGPIPE, SIG_IGN);

    bool isFirst = true;
    
    while(get_conn_status(broad_conn) != USER_STATUS_SUSPEND) {

        // 브로드캐스트 메시지를 전달하기 전까지 무한정 대기합니다.
        if(!isFirst && get_conn_status(broad_conn) == USER_STATUS_JOINED_AND_WAIT_MESSAGE) {
            pthread_mutex_lock(user_mutex);
            continue;
        }

        char buf[MAX_LENGTH_MESSAGE] = {0, };
        int bytes = SSL_read(user_broad_session, buf, MAX_LENGTH_MESSAGE);

        // TLS 통신 에러 발생시 연결을 강제 종료합니다.
        if(bytes <= 0) {
            int error = ERR_get_error();
            output_message(MSG_ERROR, conn, message_mutex, "Broadcast TLS Connection Failed, Reason: %s\n", ERR_error_string(error, NULL));
            set_conn_status(broad_conn, USER_STATUS_SUSPEND);
        }
        else {
            Message* msg = packing_message_convert(buf, bytes);
            
            // 패킷 규격에 맞지 않는 비정상적인 패킷인지 검증합니다.
            if( __builtin_expect(msg == (Message*)-1, 0)) {
                output_message(MSG_CONNECTION, conn, message_mutex, "Broadcast forcibly disconnected, packet validation failed\n");
                send_response_code(RESPONSE_CONN_BYE, conn, mutex);
                set_conn_status(broad_conn, USER_STATUS_SUSPEND);
            }

            // 연결 상태를 변경하여 메시지 전송을 위한 작업을 대기합니다.
            // broad_send_message 함수를 참고하십시오.
            if(packing_message_command_type(msg) == CMD_BROADCASE_MESSAGE) {
                set_conn_status(broad_conn, USER_STATUS_JOINED_AND_WAIT_MESSAGE);
            }
            else {
                send_response_code(RESPONSE_CONN_NO_SUPPORT, conn, mutex);
            }
            free(msg);
        }
        isFirst = false;
    }

    output_message(MSG_CONNECTION, conn, message_mutex, "Broadcast client was disconnected from chat server\n");
    user_broad_free(ctx);
}
