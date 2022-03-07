/**
 * @file comm.h
 * @author ruskonert
 * @brief Packing Message를 생성하고, 변환하고, 송신하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-15
 */

#ifndef COMM_H
#define COMM_H

#include <json.h>
#include <unistd.h>
#include <openssl/ssl3.h>
#include <stdbool.h>
#include <pthread.h>

#include "types.h"

//
// 서버와 클라이언트가 주고받는 메시지(패킷) 필드 값 및 길이를 정의합니다.
//
// 서버와 클라이언트 간 패킷 구조는 다음과 같으며, length 필드는 Message 필드의
// 길이입니다.
//
// 총 길이는 16,384 bytes이며, SSL3_RT_MAX_PLAIN_LENGTH 매크로 값을 따릅니다.
//
// ----------------------------------------------------------------------
// | Magic Number (4 bytes) | command type (2 bytes) | length (2 bytes) |
// ----------------------------------------------------------------------
// |                     Message (0 ~ 16,376 bytes)                     |
// ----------------------------------------------------------------------
//

#define MAX_LENGTH_MESSAGE SSL3_RT_MAX_PLAIN_LENGTH         /* 메시지 패킷에 대한 최대 길이, OpenSSL에서 정의한 최대 길이값에 따름 */
#define MAX_LENGTH_STR_MESSAGE (MAX_LENGTH_MESSAGE - 8)     /* 메시지의 최대 길이, 매직 넘버 및 커맨드 유형, 길이 값에 대해 제외 */
#define MAX_LENGTH_COMMAND 256                              /* 지원되는 최대 커맨드 갯수 */
#define MAGIC_NUMBER "\x99\x88\x77\x66"                     /* 매직 넘버 */
#define PROGRAM_VERSION "1.0.0"                             /* 프로그램 기본 버전 */

//
// Packing Message를 수신하고, 이에 대한 Response code를 정의합니다.
//

#define RESPONSE_CONN_OK          0x00                /* 정상 */
#define RESPONSE_CONN_WELCOME     0x01                /* 핸드쉐이크 성공 */
#define RESPONSE_CONN_NO_SUPPORT  0x02                /* 명령을 지원하지 않음 */
#define RESPONSE_CONN_BYE         0x03                /* 연결 종료 */
#define RESPONSE_CONN_SKIP        0xFE                /* 무시, ACK PACKING MESSAGE 전용 */
#define RESPONSE_CONN_FAIL        0xFF                /* 연결 실패 혹은 연결 수행 실패 */

//
// 클라이언트 또는 서버의 현재 연결 상태를 나타내는 Status code를 정의합니다.
//

#define USER_STATUS_NOT_ESTABLISHED         -1    /* 연결 아직 안됨 */
#define USER_STATUS_CONNECTION_ESTATISHED    1    /* TLS 핸드쉐이크 성공, 클라이언트 검증 안됨 */
#define USER_STATUS_JOINED_SERVER            2    /* 서버 접속 성공, 클라이언트 검증 완료  */
#define USER_STATUS_DISCONNECTED             998  /* 연결 종료 */
#define USER_STATUS_SUSPEND                  999  /* 연결 종료 대기를 위한 세션 일시중지 */

// 버퍼 문자열로부터 Response code로 변환합니다.
int response_code_convert(char* buf);

/**
 * Response code에 따른 응답 패킷을 전송합니다.
 * 
 * @param response_code Response code를 포함합니다.
 * @param conn 클라이언트 또는 서버에 대한 연결 정보를 포함합니다.
 * @param connection_mutex 연결에 대한 쓰레드 뮤텍스가 포함됩니다.
 */
void send_response_code(int response_code, Connection* conn, pthread_mutex_t* connection_mutex);


//
// 클라이언트 또는 서버 콘솔 윈도우에서, 메시지 출력을 위한 메시지 유형을
// 정의합니다.
//
enum message_type {
    MSG_CONNECTION,   /* 연결 관련 메시지 */
    MSG_MESSAGE,      /* 유저 메시지 */
    MSG_INFO,         /* 정보 메시지 */
    MSG_WARNING,      /* 경고 메시지 */
    MSG_ERROR,        /* 오류 메시지 */
    MSG_COMMAND       /* 명령(커맨드) 관련 메시지 */
};

typedef enum message_type MESSAGE_TYPE;


//////////////////////////////////////////////////////////////////
///
/// Packing Message 생성 및 변환에 대한 함수를 정의합니다.
///

/**
 * 스케쥴 메시지를 생성합니다.
 * 
 * @param maxim 할당 헤제에 도달하기 위한 카운트를 설정합니다.
 * @param message Packing Message를 포함합니다.
 * @param depend Packing Message 데이터와 종속 여부를 결정합니다.
 * 
 * @return ScheduleMessage* 생성된 스케쥴 메시지를 반환합니다. 생성 실패시 -1를 
 *         반환합니다.
 */
ScheduleMessage* schedule_message_create(int maxim, bool depend, Message* message);


// 스케쥴 메시지에 포함되어 있는 Packing Message를 반환합니다.
Message*         schedule_message_origin(ScheduleMessage* message);


// 엑세스 카운트를 1 증가시킵니다. 할당 헤제 카운트에 도달 시 자원 할당이 헤제됩니다.
void             schedule_message_increase(ScheduleMessage* message);


// Packing Message 할당을 헤제합니다. origin_msg_free을 통해 Packing Message 할당
// 헤제 여부를 결정합니다.
void             schedule_message_free(ScheduleMessage* message, bool origin_msg_free); 

///
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
///
/// Packing Message 생성 및 변환, 송수신에 대한 함수를 정의합니다.
///

/**
 * Packing Message를 생성합니다.
 * 
 * @param type 커맨드 타입 값이 포함됩니다.
 * @param len  메시지 길이가 포함됩니다. 
 * @param message 메시지 문자열이 포함됩니다.
 * @return Message* 
 */
Message*         packing_message_create(int type, int len, char* message);


/**
 * 바이너리 문자열을 Packing Message 구조체 타입으로 변환하여 인스턴스를 생성합니다.
 * 일반적으로 수신받은 데이터에 대해서 변환 시 활용할 수 있습니다.
 * 
 * @param buffer 바이너리 데이터를 포함합니다.
 * @param bytes 바이너리 데이터의 길이 값을 포함합니다.
 * @return Message* 변환된 Packing Message를 반환합니다. 변환에 실패하면 -1을 반환합니다.
 */
Message*         packing_message_convert(char* buffer, int bytes);


/**
 * json 문자열을 Packing Message 구조체 타입으로 변환하여 인스턴스를 생성합니다.
 * 일반적으로 수신받은 데이터에 대해서 변환 시 활용할 수 있습니다.
 * 
 * @param buffer json 데이터를 포함합니다.
 * @return Message* 변환된 Packing Message를 반환합니다. 변환에 실패하면 -1을 반환합니다.
 */
Message*         packing_message_convert_json(char* json_buffer);


// 커맨드 타입, 메시지 길이, 메시지 문자열을 값으로 받아 Packing Message에 대한
// 바이너리 데이터로 변환합니다.
char*            packing_message_to_string(int type, int len, char* message);


// Packing Message 구조체에 있는 커맨드 타입 값을 반환합니다.
int              packing_message_command_type(Message* message);


// Packing Message 구조체에 있는 문자열 데이터를 반환합니다.
char*            packing_message_string(Message* message);


// Packing Message 데이터를 바이너리 문자열 데이터로 변환합니다.
char*            packing_message_convert_string(Message* message);

/**
 * Packing Message를 송신합니다. 응답 루틴은 포함하지 않습니다.
 * 
 * @param conn 클라이언트 또는 서버에 대한 연결 정보를 포함합니다.
 * @param m Packine Message를 포함합니다.
 * @return int Packing Message를 송신한 후, SSL_write에 대한 오류 값을 반환합니다.
 */
int              packing_message_send(Connection* conn, Message* m);

/**
 * Packing Message를 json 형식으로 송신합니다. 응답 루틴은 포함하지 않습니다.
 * 
 * @param conn 클라이언트 또는 서버에 대한 연결 정보를 포함합니다.
 * @param m Packine Message를 포함합니다.
 * @return int Packing Message를 송신한 후, SSL_write에 대한 오류 값을 반환합니다.
 */
int              packing_message_json_send(Connection* conn, Message* m);


/**
 * Packing Message를 json 객체로 변환합니다.
 * 
 * @param message Packing message가 포함됩니다.
 * @return json_object* 변환된 json 객체가 반환됩니다.
 */
json_object*     packing_message_json(Message* message);


// Packing Message 자원 할당을 헤제합니다.
void             packing_message_free(Message* m);

///
//////////////////////////////////////////////////////////////////


/**
 * SSL 세션에 대해서 자원 할당을 헤제하고, 디스크립터 값을 읽어들어 소켓 연결을 종료하고 닫습니다.
 * 연결 상태는 USER_STATUS_DISCONNECTED로 전환합니다.
 * 
 * @param conn 클라이언트 또는 서버에 대한 연결 정보를 포함합니다.
 * @param connection_mutex 연결에 대한 쓰레드 뮤텍스가 포함됩니다.
 * @param safety_shutdown SSL_Shutdown 함수 호출을 포함하는지 결정합니다.
 * @return true 작업이 성공적이면 반환합니다.
 * @return false 작업에 실패했을 경우 반환합니다.
 */
bool             disconnect(Connection* conn, pthread_mutex_t* connection_mutex, bool safety_shutdown);


#endif