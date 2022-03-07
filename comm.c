/**
 * @file comm.c
 * @author ruskonert
 * @brief Packing Message를 생성하고, 변환하고, 송신하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-15
 */
#include "comm.h"
#include "connection.h"
#include "util.h"


// 
// Packing Message 구조체를 정의합니다.
//
struct message_t
{
    int length;                             /* 메시지 길이 */
    int commend_type;                       /* 커맨드 형식 */
    char message[MAX_LENGTH_STR_MESSAGE];   /* 메시지, 실제 데이터 */
};

// 
// 스케쥴 메시지 구조체를 정의합니다.
//
// 스케쥴 메시지는 클라이언트 혹은 서버에서 엑세스해 할당 헤제 카운트 이상이 되면
// 자동으로 할당을 헤제할 수 있는 메시지로, 브로드캐스트와 같은 여러 세션에서 여러
// 번 활용시 사용이 권장됩니다.
struct message_schedule_t
{
    int  _current;
    int  _max;
    bool _depend;
    struct message_t* msg;
};



int packing_message_json_send(Connection* conn, Message* m)
{
    json_object* jobj = packing_message_json(m); 
    const char* jobj_str = json_object_to_json_string(jobj);
    
    int err = SSL_write(get_conn_ssl_session(conn), jobj_str, strlen(jobj_str));
        
    return err;
} 



json_object *packing_message_json(Message* message)
{
    json_object *myobj = json_object_new_object();
    json_object_object_add(myobj, "length", json_object_new_int(message->length));
    json_object_object_add(myobj, "command_type", json_object_new_int(message->commend_type));
    json_object_object_add(myobj, "message", json_object_new_string_len(message->message, message->length));
    return myobj;
}



ScheduleMessage* schedule_message_create(int maxim, bool depend, Message* message)
{
    if( message == 0 ) return -1;
    ScheduleMessage* smsg = (ScheduleMessage*)malloc(sizeof(ScheduleMessage));
    smsg->_current = 0;
    smsg->_max = maxim;
    smsg->_depend = depend;
    smsg->msg = message;
    return smsg;
}



Message* schedule_message_origin(ScheduleMessage* message)
{
    return message->msg;
}



void schedule_message_increase(ScheduleMessage* message)
{
    if(message == 0) return;
    message->_current += 1;
    if(message->_max <= message->_current) {
        schedule_message_free(message, message->_depend);
    }
}



void schedule_message_free(ScheduleMessage* message, bool origin_msg_free) 
{
    packing_message_free(message->msg);
    if(origin_msg_free) free(message);
}



void packing_message_free(Message* m)
{
    free(m);
}



char* packing_message_string(Message* message)
{
    return message->message;
}



int packing_message_command_type(Message* message)
{
    return message->commend_type;
}



Message* packing_message_create(int type, int len, char* message)
{
    Message* msg = (Message*)malloc(sizeof(Message));
    msg->length = len;
    msg->commend_type = type;
    strcpy(msg->message, message);
    return msg;
}



char* packing_message_convert_string(Message* message)
{
    return packing_message_to_string(message->commend_type, message->length, message->message);
}



char* packing_message_to_string(int type, int len, char* message)
{
    char *pkt = (char*)malloc(sizeof(char) * MAX_LENGTH_MESSAGE);
    sprintf(pkt, "%s%04hx%04hx", MAGIC_NUMBER, type, len);
    strcat(pkt + 12, message);
    return pkt;
}



int packing_message_send(Connection* conn, Message* m)
{
    if( m == 0 ) return -1;

    int type = m->commend_type;
    int len = m->length;
    char* message = m->message;
    char *pkt = (char*)malloc(sizeof(char) * MAX_LENGTH_MESSAGE);
    sprintf(pkt, "%s%04hx%04hx", MAGIC_NUMBER, type, len);
    strcat(pkt + 12, message);
    
    int err = SSL_write(get_conn_ssl_session(conn), pkt, len + 12);
    free(pkt);
    
    return err;
}
 


Message* packing_message_convert(char* buffer, int bytes)
{
    // 바이너리 데이터의 길이를 검증합니다.
    if(bytes <= 0) return (Message*)-1;
    if(bytes >= 0 && bytes < 8) return (Message*)-1;

    char* magic_number = substring(buffer, 1, 4);
    if ( strcmp(MAGIC_NUMBER, magic_number) != 0) {
        free(magic_number);
        return (Message*)-1;
    }
    free(magic_number);

    // 커맨드 타입 값을 가져옵니다.
    char* command_s = substring(buffer, 5, 4);
    int command = strtol(command_s,NULL,16);
    free(command_s);

    // 메시지 길이 값을 가져옵니다.
    char* length_s = substring(buffer, 9, 4);
    int length = strtol(length_s, NULL, 16);
    free(length_s);

    if( length > MAX_LENGTH_STR_MESSAGE) return (Message*)-1;
    
    char* message = 0;
    Message* msg = 0;
    if(length == 0)
    {
        message = "\0";
        msg = (Message*)malloc(sizeof(Message));
        strcpy(msg->message, message);
    }
    else
    {
        message = substring(buffer, 13, length);
        message[length] = '\0';
        msg = (Message*)malloc(sizeof(Message));
        strcpy(msg->message, message);
        free(message);
    }

    msg->commend_type = command;
    msg->length = length;
    return msg;
}


Message* packing_message_convert_json(char* json_buffer)
{
    json_object *jobj = json_tokener_parse(json_buffer);
    json_object *length_obj = json_object_object_get(jobj, "length");
    json_object *message_obj = json_object_object_get(jobj, "message");
    json_object *command_type_obj = json_object_object_get(jobj, "command_type");
    
    Message* message = packing_message_create(json_object_get_int(command_type_obj), json_object_get_int(length_obj), (char*)json_object_get_string(message_obj));
    json_object_put(jobj);

    return message;
}


int response_code_convert(char* buf)
{
    if( strcmp("OK", buf) == 0 ) return RESPONSE_CONN_OK;
    if( strcmp("WELCOME", buf) == 0 ) return RESPONSE_CONN_WELCOME;
    if( strcmp("FAIL", buf) == 0 ) return RESPONSE_CONN_FAIL;
    if( strcmp("NOSUPPORT", buf) == 0 ) return RESPONSE_CONN_NO_SUPPORT;
    if( strcmp("SKIP", buf) == 0 ) return RESPONSE_CONN_SKIP;
    if( strcmp("BYE", buf) == 0 ) return RESPONSE_CONN_BYE;
    return RESPONSE_CONN_SKIP;
}



void send_response_code(int status_code, Connection* conn, pthread_mutex_t* mutex)
{
    pthread_mutex_lock(mutex);

    SSL* session = get_conn_ssl_session(conn);

    // Response code 별로 응답합니다.
    switch( status_code ) {
        case RESPONSE_CONN_FAIL:
            SSL_write(session, "FAIL", strlen("FAIL"));
            break;
        case RESPONSE_CONN_OK:
            SSL_write(session, "OK", strlen("OK"));
            break;
        case RESPONSE_CONN_WELCOME:
            SSL_write(session, "WELCOME", strlen("WELCOME"));
            break;
        case RESPONSE_CONN_NO_SUPPORT:
            SSL_write(session, "NOSUPPORT", strlen("NOSUPPORT"));
            break;
        case RESPONSE_CONN_BYE:
            SSL_write(session, "BYE", strlen("BYE"));
            set_conn_status(conn, USER_STATUS_SUSPEND);
            break;
        /* RESPONSE_CONN_SKIP은 응답하지 않고, 응답을 별도로 수행해야 합니다. */
    }
    pthread_mutex_unlock(mutex);
}



bool disconnect(Connection* conn, pthread_mutex_t* connection_mutex, bool safety_shutdown)
{
    SSL* session = get_conn_ssl_session(conn);
    if(session == 0) {
        return false;
    }

    pthread_mutex_lock(connection_mutex);

    if(safety_shutdown) SSL_shutdown(session);
    SSL_free(session);

    SOCKET_HANDLE sd = get_conn_socket_id(conn);
    if(sd > 0) close(get_conn_socket_id(conn));

    set_conn_status(conn, USER_STATUS_DISCONNECTED);

    pthread_mutex_unlock(connection_mutex);
    return true;
}