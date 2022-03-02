/**
 * @file types.h
 * @author ruskonert
 * @brief 채팅 프로그램을 구현하는 데 있어 사용하는 기본 타입, 전역 구조체를 정의합니다.
 * @version 1.0
 * @date 2022-02-14
 */
#ifndef TYPES_H
#define TYPES_H

/*
 * 클라이언트 및 서버 간 연결을 담당하는 구조체를 정의합니다.
 */

struct connection_t;

typedef struct connection_t Connection;

/*
 * SSL 및 Certificate 관련 정보를 저장하는 구조체를 정의합니다.
 */

struct ssl_ctx_t;
struct ssl_cert_ctx_t;

typedef struct ssl_ctx_t SSLContext;
typedef struct ssl_cert_ctx_t SSLCertContext;

/*
 * 서버와 클라이언트 간 송수신 시 사용하는 메시지 구조체를 정의합니다. 
 */

struct message_t;
struct message_schedule_t;

typedef struct message_t Message;
typedef struct message_schedule_t ScheduleMessage;

// 커맨드를 실행하는 함수 type을 정의합니다.
typedef int (*COMMAND_FUNC)(void*, Message*);


// 소켓 디스크립터에 대한 타입을 정의합니다. 
typedef int SOCKET_HANDLE; 

#endif