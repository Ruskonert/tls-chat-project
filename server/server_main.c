/**
 * @file server_main.h
 * @author ruskonert
 * @brief 서버 프로그램의 main 함수를 구현합니다.
 * @version 1.0
 * @date 2022-02-18
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "../util.h"

#include "server_main.h"
#include "server_net.h"

#define REFRESH_TIME 120 /* 인원 메시지 출력 간격 */

extern void initialize_command_loader(COMMAND_FUNC *);


// 사용자 명령 별 함수를 담고 있는 배열을 전역 정의합니다.
COMMAND_FUNC FUNC_ARR[MAX_LENGTH_COMMAND];


COMMAND_FUNC* get_prog_command_function() { return FUNC_ARR; }


// 현재 채팅 서버에 접속한 인원 정보 메시지를 REFRESH_TIME초 간격으로 출력합니다. 
void* output_current_user(void* ctx)
{
    ConnectManager* cm = (ConnectManager*)ctx;
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(cm);
    while(1) {
        sleep(REFRESH_TIME);
        output_message(MSG_INFO, NULL, message_mutex, "채팅 서버 내 접속 인원: [%d/%d]\n", get_current_joined_user(cm), MAX_USER_CONNECTION);
    }
    return 0;
}


int main(int argc, char* argv[])
{
    char* addr;
    int port;

    // 인자가 없다면 기본 주소 및 포트를 사용합니다.
    // 기본 포트는 4433번입니다.
    if(argc < 2) {
        output_message(MSG_INFO, NULL, NULL, "You can also use as follows: %s [host_ip] [port]\n", argv[0]);
        addr = "0.0.0.0";
        port = 4433;
    }
    else {
        addr = argv[1];
        port = atoi(argv[2]);
    }

    pthread_t thread;
    initialize_command_loader(FUNC_ARR);


    ConnectManager *ctx = connect_manager_create(true);
    if( !connect_manager_execute(ctx, addr, port) ) {
        printf("Creating context was failed\n");
        return -1;
    }
    pthread_create(&thread, NULL, output_current_user, (void*)ctx);
    ConnectManager *broad_ctx = connect_manager_create(false);
    if( !create_broad_session_job(ctx, addr, 8443)) {
        printf("Creating broadcast context was failed\n");
        return -1;
    }

    // 현재 접속한 유저와 연결을 끊습니다.
    UserContext** user_arr = get_connect_manager_user(ctx);

    for(int i = 0; i < MAX_USER_CONNECTION; i++)
    {
        user_disconnect(user_arr[i], true);
    }

    SSLContext* broad_context = get_conn_ssl_context(get_connect_manager_of_conn(broad_ctx));
    SSLContext* sever_context = get_conn_ssl_context(get_connect_manager_of_conn(ctx));

    SSL_CTX_free(get_ssl_ctx_context(broad_context));
    SSL_CTX_free(get_ssl_ctx_context(sever_context));

    connect_manager_free(ctx);

    return 0;
}