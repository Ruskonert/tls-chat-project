/**
 * @file connection.h
 * @author ruskonert
 * @brief 클라이언트, 서버에 대한 네트워크 연결 정보를 생성하는 기능, 구조체를 정의합니다.
 * @version 1.0
 * @date 2022-02-14
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <openssl/ssl.h>
#include <pthread.h>
#include <stdbool.h>

#include "types.h"



/**
 * @brief 클라이언트 연결에 대한 SSL 컨텍스트를 생성합니다. SSL 세션에 대한 생성을 담지 않습니다.
 * 
 * @param isInitialized OpenSSL 라이브러리 내 알고리즘 및 오류 정보를 로드할 것인지 결정합니다. 
 *                      일반적으로 초기 과정 중 한번만 로드합니다.
 * 
 * @return 성공 시 SSL 컨텍스트를 반환합니다. 그렇지 않다면, -1을 반환합니다.
 */
SSLContext*       create_client_ssl_context(bool isInitialized);


/**
 * @brief 서버 연결에 대한 SSL 컨텍스트를 생성합니다. SSL 세션에 대한 생성을 담지 않습니다.
 * 
 * @param isInitialized OpenSSL 라이브러리 내 알고리즘 및 오류 정보를 로드할 것인지 결정합니다. 
 *                      일반적으로 초기 과정 중 한번만 로드합니다.
 * 
 * @return 성공 시 SSL 컨텍스트를 반환합니다. 그렇지 않다면, -1을 반환합니다.
 */
SSLContext*       create_server_ssl_context(bool isInitialized);


//////////////////////////////////////////////////////////////////
///
/// SSLContext에 대한 구조체 getter를 정의합니다. setter는 정의되지 않습니다.
///
const SSL_METHOD* get_ssl_ctx_method(SSLContext* ctx);

SSL_CTX*          get_ssl_ctx_context(SSLContext* ctx);
///
//////////////////////////////////////////////////////////////////


/**
 * @brief SSL 컨텍스트에 기반하여 통신하는 클라이언트 또는 서버의 네트워크 상태 정보, 세션을 담을 수 있는 
 *        Connection 인스턴스를 생성합니다.
 * 
 * @param _ssl SSL 컨텍스트가 포함됩니다.
 * @return Connection* 새로 생성된 Connection 인스턴스를 반환합니다. 생성 실패 시 -1를 반환합니다.
 */
Connection*       connection_create(SSLContext* _ssl);


//////////////////////////////////////////////////////////////////
///
/// Connection에 대한 구조체 getter 및 setter를 정의합니다.
///
SSLContext*       get_conn_ssl_context(Connection* conn);

SSL*              get_conn_ssl_session(Connection* conn);
void              set_conn_ssl_session(Connection* conn, SSL* ssl);

SOCKET_HANDLE     get_conn_socket_id(Connection* conn);
void              set_conn_socket_id(Connection* conn, SOCKET_HANDLE sd);

void              set_conn_ip_addr(Connection* conn, const char* addr);
const char*       get_conn_ip_addr(Connection* conn);

int               get_conn_status(Connection* conn);
void              set_conn_status(Connection* conn, int status);

///
//////////////////////////////////////////////////////////////////


// Connection 구조체에 대한 할당을 헤제합니다.
void conn_ctx_free(Connection* conn);


#endif