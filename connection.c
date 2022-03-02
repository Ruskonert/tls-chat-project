/**
 * @file connection.c
 * @author ruskonert
 * @brief 클라이언트, 서버에 대한 네트워크 연결 정보를 생성하는 기능, 구조체를 정의합니다.
 * @version 1.0
 * @date 2022-02-14
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <openssl/err.h>

#include "connection.h"
#include "util.h"


// 네트워크 연결과 관련된 부속 정보를 담는 구조체를 정의합니다.
struct connection_t
{
    int status;                    /* 연결 상태 */
    char addr[16];                 /* IP 정보 */

    SSL* _ssl;                     /* SSL 세션 */
    SOCKET_HANDLE _sd;             /* 소켓 디스크립터 값 */

    struct ssl_ctx_t* _ssl_c_ctx; 
};


// 서버 및 클라이언트가 사용하는 SSL cert 정보를 정의합니다.
struct ssl_cert_ctx_t
{
    char _cert[PATH_MAX];          /* 인증서 파일 경로 */
    char _key[PATH_MAX];           /* 개인키 파일 경로 */
};


// SSL 메소드 및 컨텍스트를 담는 구조체를 정의합니다.
struct ssl_ctx_t
{
    const SSL_METHOD       *method;  
    SSL_CTX                *ctx; 
    struct ssl_cert_ctx_t   cert_ctx;
};


//////////////////////////////////////////////////////////////////
///
/// SSLContext에 대한 구조체 getter를 정의합니다. setter는 정의되지 않습니다.
///

const SSL_METHOD* get_ssl_ctx_method(SSLContext* ctx) { return ctx->method; }

SSL_CTX*          get_ssl_ctx_context(SSLContext* ctx) { return ctx->ctx; }


///
//////////////////////////////////////////////////////////////////


Connection* connection_create(SSLContext* _ssl)
{
    if(_ssl == 0) return -1;

    Connection* conn = (Connection*)malloc(sizeof(Connection));
    memset(conn, 0, sizeof(Connection));

    conn->_ssl_c_ctx = _ssl;
    conn->status = -1;
    
    return conn;
}



SSLContext* create_server_ssl_context(bool isInitialized)
{
    if(isInitialized) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
    }

    SSLContext* ctx = (SSLContext*)malloc(sizeof(SSLContext));

    // TLS 알고리즘 최신 버전을 채택합니다.
    ctx->method = TLS_server_method();
    
    SSL_CTX* ssl_ctx = SSL_CTX_new(ctx->method);

    // 키 및 인증서에 대한 경로를 설정합니다.
    const char* cert_file = "./cert/server.crt";
    const char* key_file = "./cert/server.key";

    if ( SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        return (SSLContext*)-1;
    }

    // 키 파일에서 개인 키를 설정합니다. (CertFile과 동일할 수 있음)
    if ( SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        return (SSLContext*)-1;
    }

    // 개인키를 검증합니다.
    if ( !SSL_CTX_check_private_key(ssl_ctx) ) {
        output_message(MSG_ERROR, NULL, NULL, "[%s] 공개 인증서와 개인키가 매칭되지 않습니다\n", __FUNCTION__);
        return (SSLContext*)-1;
    }

    struct ssl_cert_ctx_t cert_ctx;
    strcpy(cert_ctx._cert, cert_file);
    strcpy(cert_ctx._key, key_file);

    ctx->cert_ctx = cert_ctx;
    ctx->ctx = ssl_ctx;
    return ctx;
}



SSLContext* create_client_ssl_context(bool isInitialized)
{
    if(isInitialized) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
    }

    SSLContext* ctx = (SSLContext*)malloc(sizeof(SSLContext));

    // TLS 알고리즘 최신 버전을 채택합니다.
    ctx->method = TLS_client_method();

    SSL_CTX* ssl_ctx = SSL_CTX_new(ctx->method);
    ctx->ctx = ssl_ctx;

    return ctx;
}


//////////////////////////////////////////////////////////////////
///
/// Connection 구조체의 getter 및 setter를 정의합니다.
///

SSLContext*       get_conn_ssl_context(Connection* conn) { return conn->_ssl_c_ctx; }

SSL*              get_conn_ssl_session(Connection* conn) { return conn->_ssl; }
void              set_conn_ssl_session(Connection* conn, SSL* ssl) { conn->_ssl = ssl; }


SOCKET_HANDLE     get_conn_socket_id(Connection* conn) { return conn->_sd; }
void              set_conn_socket_id(Connection* conn, SOCKET_HANDLE sd) { conn->_sd = sd; }


void              set_conn_ip_addr(Connection* conn, const char* addr) { strcpy(conn->addr, addr); }
const char*       get_conn_ip_addr(Connection* conn) { return conn->addr; }


int               get_conn_status(Connection* conn) { return conn->status; }
void              set_conn_status(Connection* conn, int status) { conn->status = status; }

///
//////////////////////////////////////////////////////////////////

void conn_ctx_free(Connection* conn)
{
    /* SSL 세션, SSL 컨텍스트, 소켓 디스크립터는 헤제하지 않습니다.
       이는 별도로 헤제되어야 합니다. */
    free(conn);
}
