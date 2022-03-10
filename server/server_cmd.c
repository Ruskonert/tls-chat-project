/**
 * @file server_cmd.c
 * @author ruskonert
 * @brief 클라이언트에서 요청한 명렁을 처리하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-21
 */

#include <unistd.h>
#include <string.h>
#include <strings.h>

#include "server_net.h"
#include "server_broad.h"

#include "../comm.h"
#include "../connection.h"
#include "../util.h"
#include "../cmd.h"


/**
 * 유저에게 응답하기 위한 Packing Message를 전송합니다.
 * 
 * 이 함수가 커맨드 함수에서 호출되었다면, 커맨드 함수의 반환값은 
 * RESPONSE_CONN_SKIP이어야 합니다.
 * 
 * 만약 그렇지 않다면, 문제가 발생할 수 있습니다.
 * 
 * @param user 보내고자 하는 유저 컨텍스트가 포함됩니다.
 * @param message Packing Message가 포함됩니다.
 * @return int SSL_write에 대한 반환값입니다.
 */
int send_ack_packing_message(User* user, Message* message)
{
    Connection* conn = get_user_conn(user);
    int status =  packing_message_send(conn, message);

    packing_message_free(message);
    return status;
}

struct broad_arg_t
{
    User* user;
    ScheduleMessage* message;
};


// 현재 서버에 접속하고 있는 모든 인원에게 특정 메시지를 전송할 떄 실행됩니다.
int cmd_broadcast_message(void* ctx, Message* message)
{
    User* user = (User*)ctx;
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(get_user_connect_manager(user));
    Connection* conn = get_user_conn(user);

    output_message(MSG_INFO, conn, mutex, "%s said: %s\n", get_user_name(user), packing_message_string(message));

    ConnectManager* cm = get_user_connect_manager(user);
    User** connected_user_arr = get_connect_manager_user(cm);

    char buf[MAX_LENGTH_MESSAGE] = {0,};
    memset(buf, 0, MAX_LENGTH_MESSAGE);

    sprintf(buf, "[%s]: %s", get_user_name(user), packing_message_string(message));

    Message* c_message = packing_message_create(1, strlen(buf), buf);
    ScheduleMessage* s_message = schedule_message_create(get_current_joined_user(cm), true, c_message);

    for(int i = 0; i < MAX_USER_CONNECTION; i++) {
        User* o_user = connected_user_arr[i];

        if(o_user == 0) continue;
        if(! is_user_joined_server(o_user)) continue;
        
        pthread_t thread;

        struct broad_arg_t* a_list = (struct broad_arg_t*)malloc(sizeof(struct broad_arg_t));

        a_list->user = o_user;
        a_list->message = s_message;
        pthread_create(&thread, NULL, broad_send_message, (void*)a_list);
    }
    return RESPONSE_CONN_OK;
}


// 클라이언트와 처음 연결한 후, 클라이언트 검증 과정을 수행하는 함수입니다.
int cmd_welcome_handshake(void* ctx, Message* message)
{
    User* user = (User*)ctx;
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(get_user_connect_manager(user));
    Connection* conn = get_user_conn(user);

    char* str_message = packing_message_string(message);
    char client_prefix[32], client_version[32];

    char *ptr = strtok(str_message, " ");

    strcpy(client_prefix, ptr);
    strcpy(client_version, ptr + 15);

    if(strcmp(client_prefix, "CLIENT_VERSION") != 0) return RESPONSE_CONN_FAIL;
    if(strcmp(client_version, PROGRAM_VERSION) != 0) return RESPONSE_CONN_FAIL;



    int user_idx = get_user_allocated_index(user);
    if( user_idx + 1 > MAX_USER_CONNECTION) { /* 접속한 인원이 MAX_USER_CONNECTION보다 많으면, 초과하는 인원에 대해서
                                                 연결을 중지해야 합니다. */
        output_message(MSG_ERROR, conn, message_mutex, "The user is full, Communication failed\n");

        char* _m = "참여 가능 인원이 없습니다. 인원이 꽉찼습니다.\n";
        Message* msg = packing_message_create(CMD_CHANGE_NICKNAME, strlen(_m), _m);
        send_ack_packing_message(user, msg);

        set_conn_status(conn, USER_STATUS_SUSPEND);
        return RESPONSE_CONN_SKIP;
    }


    if(get_conn_status(conn) == USER_STATUS_CONNECTION_ESTATISHED) {
        set_user_verified(user, true); /* 클라이언트가 핸드쉐이크를 정상적으로 수행했으므로, 참여 상태를 변경합니다. */
        output_message(MSG_INFO, conn, message_mutex, "Client hello, Client version: %s\n", client_version);

        set_conn_status(conn, USER_STATUS_JOINED_SERVER);

        char buf[MAX_LENGTH_STR_MESSAGE] = {0};
        sprintf(buf, "채팅 서버에 참가하였습니다!");
        Message* c_message = packing_message_create(1, strlen(buf), buf);
        cmd_broadcast_message(ctx, c_message);
        return RESPONSE_CONN_WELCOME;
    }
    else {
        output_message(MSG_WARNING, conn, message_mutex, "This client was alrady estabilished, but retrying, skipping\n");
        return RESPONSE_CONN_OK;
    }
}


// 클라이언트에서 닉네임 변경을 요청했을 때 수행되는 함수입니다.
int cmd_change_name(void* ctx, Message* message)
{
    char buf[512] = {0, };
    User* user = (User*)ctx;
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(get_user_connect_manager(user));


    if( __builtin_expect(!is_user_verified(user), 0)) return RESPONSE_CONN_FAIL;
    

    if(strlen(packing_message_string(message)) == 0) {
        output_message(MSG_COMMAND, get_user_conn(user), mutex, "Requested username changed, but name is empty\n");

        char _m[64] = {0,};
        sprintf(_m, "현재 닉네임 - %s", get_user_name(user));
        Message* message = packing_message_create(CMD_CHANGE_NICKNAME, strlen(_m), _m);
        send_ack_packing_message(user, message);

        return RESPONSE_CONN_SKIP;
    }

    char* nickname = packing_message_string(message);

    if( USERNAME_MAX_LENGTH <= strlen(nickname)) {
        char* _m = "바꾸고자 하는 닉네임은 63자를 초과할 수 없습니다.";
        Message* message = packing_message_create(CMD_CHANGE_NICKNAME, strlen(_m), _m);
        send_ack_packing_message(user, message);

        return RESPONSE_CONN_SKIP;
    }


    if(strchr(nickname, ' ') != NULL || strchr(nickname, '\"') != NULL) {
        output_message(MSG_ERROR, get_user_conn(user), mutex, "Nickname was violated the rule\n");
        char* _m = "사용이 금지된 문자가 포함되어 있습니다 (띄어쓰기, \"), 사용법: /name <바꿀이름>";
        Message* message = packing_message_create(CMD_CHANGE_NICKNAME, strlen(_m), _m);
        send_ack_packing_message(user, message);

        return RESPONSE_CONN_SKIP;
    }

    char* username = get_user_name(user);

    User** u = get_connect_manager_user(get_user_connect_manager(user));
    for(int i = 0; i < MAX_USER_CONNECTION; i++)
    {
        if(u == 0 || ! is_user_joined_server(u[i])) continue;
        if(u[i] == user) continue;

        char* name = get_user_name(u[i]);

        if(strcasecmp(nickname, name) == 0) {
            output_message(MSG_ERROR, get_user_conn(user), mutex, "Nickname is already in use\n");

            char* _m = "이미 사용중인 닉네임입니다.";
            Message* msg = packing_message_create(CMD_CHANGE_NICKNAME, strlen(_m), _m);
            send_ack_packing_message(user, msg);

            return RESPONSE_CONN_SKIP;
        }
    }

    output_message(MSG_COMMAND, get_user_conn(user), mutex, "Nickname changed [%s] -> [%s]\n", get_user_name(user), packing_message_string(message));

    sprintf(buf, "%s 님이 닉네임을 다음과 같이 변경하였습니다 - [%s]", get_user_name(user), packing_message_string(message));
    set_user_name(user, packing_message_string(message));
    Message* c_message = packing_message_create(1, strlen(buf), buf);

    return cmd_broadcast_message(user, c_message);
}


// 1:1 메시지를 송신하는 함수입니다.
int cmd_secret_message(void* ctx, Message* message)
{
    User* user = (User*)ctx;

    ConnectManager* cm = get_user_connect_manager(user);
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(cm);
    Connection* conn = get_user_conn(user);

    const char* delim = " ";

    if(strlen(packing_message_string(message)) == 0) {
        char* _m = "정보가 없습니다, 사용법: /send <사용자 닉네임> <메시지>";
        Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
        send_ack_packing_message(user, msg);
        return RESPONSE_CONN_SKIP;
    }

    char* temp_string = packing_message_string(message);
    char* recv_username = strsep(&temp_string, delim);
    char* send_message = temp_string;

    if(strlen(recv_username) == 0) {
        char* _m = "보내고자 하는 유저가 없습니다, 사용법: /send <사용자 닉네임> <메시지>";
        Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
        send_ack_packing_message(user, msg);
        return RESPONSE_CONN_SKIP;
    }


    if(send_message == 0 || strlen(send_message) == 0) {
        char* _m = "보내고자 하는 메시지가 없습니다, 사용법: /send <사용자 닉네임> <메시지 ...>";
        Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
        send_ack_packing_message(user, msg);
        return RESPONSE_CONN_SKIP;
    }

    if(strcasecmp(recv_username, get_user_name(user)) == 0) {
        char* _m = "자기 자신한테는 1:1 메시지를 보낼 수 없습니다.";
        Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
        send_ack_packing_message(user, msg);
        return RESPONSE_CONN_SKIP;
    }


    output_message(MSG_INFO, conn, mutex, "(Requested) %s said to %s: %s\n", get_user_name(user), recv_username, send_message);

    User** connected_user_arr = get_connect_manager_user(cm);

    char buf[MAX_LENGTH_MESSAGE] = {0.};
    memset(buf, 0, MAX_LENGTH_MESSAGE);

    for(int i = 0; i < MAX_USER_CONNECTION; i++) {
        User* o_user = connected_user_arr[i];
        
        if(o_user == 0) continue;
        if(o_user == user) continue;
        if(! is_user_joined_server(o_user)) continue;

        else {
            if(strcasecmp(recv_username, get_user_name(o_user)) == 0) {
                if(get_user_broad_conn(o_user) == 0) {

                    char* _m = "해당 유저는 현재 메시지 수신기가 켜져있지 않습니다. 메시지를 전달할 수 없습니다.";
                    Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
                    send_ack_packing_message(user, msg);
                    return RESPONSE_CONN_SKIP;
                }

                sprintf(buf, "(1:1 메시지) [%s]: %s", get_user_name(user), send_message);

                Message* c_message = packing_message_create(1, strlen(buf), buf);
                ScheduleMessage* s_message = schedule_message_create(2, true, c_message);
                
                pthread_t thread;
                struct broad_arg_t* a_list = (struct broad_arg_t*)malloc(sizeof(struct broad_arg_t));

                a_list->user = o_user;
                a_list->message = s_message;
                pthread_create(&thread, NULL, broad_send_message, (void*)a_list);


                pthread_t thread2;
                struct broad_arg_t* a_list2 = (struct broad_arg_t*)malloc(sizeof(struct broad_arg_t));

                a_list2->user = user;
                a_list2->message = s_message;
                pthread_create(&thread2, NULL, broad_send_message, (void*)a_list2);

                return RESPONSE_CONN_OK;
            }
        }
    }

    char* _m = "해당 유저를 찾을 수 없습니다.";
    Message* msg = packing_message_create(CMD_SECRET_MESSAGE, strlen(_m), _m);
    send_ack_packing_message(user, msg);
    
    return RESPONSE_CONN_SKIP;
}


/// 현재 서버에 접속한 모든 인원의 정보를 송신하는 함수입니다.
int cmd_current_user(void* ctx, Message* message)
{
    User* user = (User*)ctx;
    ConnectManager* cm = get_user_connect_manager(user);
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(cm);

    if( __builtin_expect(!is_user_verified(user), 0)) return RESPONSE_CONN_FAIL;

    User** user_list = get_connect_manager_user(cm);

    char _t[MAX_LENGTH_STR_MESSAGE] = {0,};
    char _m[MAX_LENGTH_STR_MESSAGE / 2] = {0,};

 
    // 클라이언트엔 다음과 같이 출력됩니다:
    //
    // [1] 닉네임
    // [2] 닉네임
    // [3] 닉네임
    // ...
    for(int i = 0; i < MAX_USER_CONNECTION; i++) {
        User* o_user = user_list[i];
        if(! is_user_joined_server(o_user)) continue;

        char* username = get_user_name(o_user);

        char buf2[512] = {0,};
        sprintf(buf2, "[%d] %s", i+1, username);

        // 본인에 대한 표시를 추가합니다.
        if (user == o_user) {
            strcat(buf2, " <- you");
        }

        if( __builtin_expect((i == 0), 0) ) {
            strcpy(_m, buf2);
        }
        else {
            strcat(_m, buf2);
        }

        if(i + 1 < MAX_USER_CONNECTION) {
            strcat(_m, "\n");
        }
    }

    sprintf(_t, "현재 접속한 인원 정보 [%d/%d]\n", get_current_joined_user(cm), MAX_USER_CONNECTION);
    strcat(_t, _m);

    Message* msg = packing_message_create(CMD_STATUS, strlen(_t), _t);
    send_ack_packing_message(user, msg);
    
    return RESPONSE_CONN_SKIP;
}


// 클라이언트에서 연결 종료를 요청했을 때 수행되는 함수입니다.
int cmd_disconnect(void* ctx, Message* message)
{
    User* user = (User*)ctx;
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(get_user_connect_manager(user));

    set_conn_status(get_user_conn(user), USER_STATUS_SUSPEND);
    output_message(MSG_COMMAND, get_user_conn(user), mutex, "Requesting disconnect to server\n");

    char buf[MAX_LENGTH_MESSAGE] = {0};
    sprintf(buf, "채팅 서버에서 나갔습니다.");
    Message* c_message = packing_message_create(1, strlen(buf), buf);

    cmd_broadcast_message(user, c_message);
    return RESPONSE_CONN_BYE;
}


// 클라이언트가 지원하지 않은 명령을 요청할 때 수행되는 함수입니다.
int cmd_not_supported(void* ctx, Message* message)
{
    User* user = (User*)ctx;
    pthread_mutex_t* mutex = get_connect_manager_of_message_mutex(get_user_connect_manager(user));
    output_message(MSG_COMMAND, get_user_conn(user), mutex, "Not support command: \"%s\"\n", packing_message_string(message));

    return RESPONSE_CONN_NO_SUPPORT;
}


/**
 * @brief 서버에서 지원하는 커맨드 함수를 가져옵니다.
 * 
 * @param FUNC_ARR 함수형 포인터 배열이 포함되며, 초기화되지 않은 배열입니다.
 */
void initialize_command_loader(COMMAND_FUNC *FUNC_ARR)
{   
    // 커맨드 타입 값에 따라 함수를 맵핑합니다.
    FUNC_ARR[CMD_HANDSHAKE] = cmd_welcome_handshake;
    FUNC_ARR[CMD_BROADCASE_MESSAGE] = cmd_broadcast_message;
    FUNC_ARR[CMD_CHANGE_NICKNAME] = cmd_change_name;
    FUNC_ARR[CMD_SECRET_MESSAGE] = cmd_secret_message;
    FUNC_ARR[CMD_STATUS] = cmd_current_user;
    FUNC_ARR[CMD_DISCONNECT] = cmd_disconnect;
    FUNC_ARR[CMD_NOT_SUPPORTED] = cmd_not_supported;
}