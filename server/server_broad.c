/**
 * @file server_broad.h
 * @author ruskonert
 * @brief 클라이언트의 메시지 수신기를 대상으로 연결을 수행하는 기능을 구현합니다.
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

struct broad_arg_t
{
    UserContext* user;
    ScheduleMessage* message;
};



void* broad_send_message(void* argv)
{
    struct broad_arg_t* argv_ptr = (struct broad_arg_t*)argv;

    UserContext* user = argv_ptr->user;
    ScheduleMessage* smessage = argv_ptr->message;

    if(user == 0) return -1;

    ConnectManager* broad_cm = get_user_broad_connect_manager(user);
    Connection* broad_conn = get_user_broad_conn(user);

    if(broad_conn == 0) {
        schedule_message_increase(smessage);
        return -1;
    }

    pthread_mutex_t* mutex = get_connect_manager_of_mutex(broad_cm);
    pthread_mutex_t* user_mutex = get_user_mutex(user);
    
    pthread_mutex_lock(mutex);
    Message* message = schedule_message_origin(smessage);

    // 메시지 전송 수행 이후, 스케줄 메시지의 카운트를 증가시킵니다.
    packing_message_send(broad_conn, message);
    schedule_message_increase(smessage);
    
    pthread_mutex_unlock(mutex);

    // 전송이 완료되면, 뮤텍스를 헤제합니다.
    // communicate_broad_user 함수에서 패킷에 대한 read 작업을
    // 다시 수행할 수 있습니다.
    pthread_mutex_unlock(user_mutex);
    free(argv_ptr);
    return 0;
}


/* 메시지 수신기와 브로드캐스트 서버와의 연결을 관리하고 패킷을 처리합니다. */
void* communicate_broad_user(void* argv)
{
    UserContext* ctx = (UserContext*)argv;

    ConnectManager* broad_cm = get_user_broad_connect_manager(ctx);

    pthread_mutex_t* broad_mutex = get_connect_manager_of_mutex(broad_cm);
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(broad_cm);
    pthread_mutex_t* user_mutex = get_user_mutex(ctx);

    Connection* conn = get_user_conn(ctx);
    Connection* broad_conn = get_user_broad_conn(ctx);
    SSL* user_broad_session = get_conn_ssl_session(broad_conn);

    output_message(MSG_CONNECTION, conn, message_mutex, "Connected the broadcast channel\n");
    set_conn_status(broad_conn, USER_STATUS_JOINED_SERVER);
    show_certs(ctx, user_broad_session);
    

    while(get_conn_status(broad_conn) != USER_STATUS_SUSPEND) {

        // 클라이언트 메시지 수신기에 데이터를 송신하기 전까지 대기합니다.
        // SSL_read 작업이 2번 수행되는 것을 방지합니다.
        pthread_mutex_lock(user_mutex);

        pthread_mutex_lock(broad_mutex);

        if(is_broad_suspend(ctx)) {
            set_conn_status(broad_conn, USER_STATUS_SUSPEND);
            pthread_mutex_unlock(broad_mutex);
            break;
        }
        else {
            signal(SIGPIPE, SIG_IGN);

            pthread_mutex_unlock(broad_mutex);

            char buf[MAX_LENGTH_MESSAGE] = {0, };
            int bytes = SSL_read(user_broad_session, buf, MAX_LENGTH_MESSAGE);

            // TLS 통신 에러 발생시 연결을 강제 종료합니다.
            if(bytes <= 0) {
                int error = ERR_get_error();
                output_message(MSG_ERROR, conn, message_mutex, "Broadcast TLS Connection Failed, Reason: %s\n", ERR_error_string(error, NULL));
                set_conn_status(broad_conn, USER_STATUS_SUSPEND);
                pthread_mutex_unlock(user_mutex);

                user_broad_disconnect(ctx, false);
                return -1;
            }

            else {
                Message* msg = packing_message_convert(buf, bytes);
                
                // 패킷 규격에 맞지 않는 비정상적인 패킷인지 검증합니다.
                if( __builtin_expect(msg == (Message*)-1, 0)) {
                    output_message(MSG_CONNECTION, conn, message_mutex, "Broadcast TLS connection forcibly disconnected, packet validation failed\n");
                    send_response_code(RESPONSE_CONN_BYE, conn, broad_mutex);
                    set_conn_status(broad_conn, USER_STATUS_SUSPEND);
                    pthread_mutex_unlock(user_mutex);

                    user_broad_disconnect(ctx, false);
                    return -1;
                }

                if(!packing_message_command_type(msg) == CMD_BROADCASE_MESSAGE) {
                    send_response_code(RESPONSE_CONN_NO_SUPPORT, conn, broad_mutex);
                    pthread_mutex_unlock(user_mutex);
                }
                free(msg);
            }
        }
    }
    user_broad_disconnect(ctx, true);
    return 0;
}
