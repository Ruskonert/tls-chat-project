/**
 * @file server_net.h
 * @author ruskonert
 * @brief TLS 핸드쉐이크 과정을 구현하고, 클라이언트 연결 정보를 관리하기 위한 기능, 
 *        구조체를 정의합니다.
 * @version 1.0
 * @date 2022-02-17
 * 
 */

#ifndef SERVER_NET_H
#define SERVER_NET_H

#include <pthread.h>
#include <stdbool.h>
#include <openssl/ssl.h>

#include "types.h"
#include "../connection.h"

struct user_t;
struct connect_manager_t;

typedef struct user_t UserContext;
typedef struct connect_manager_t ConnectManager;


//////////////////////////////////////////////////////////////////
///
/// User에 대한 getter, setter, 응용 기능에 대한 함수를 정의합니다.
///

#define MAX_USER_CONNECTION  64             /* 유저 최대 연결 가능 개수 */
#define USERNAME_MAX_LENGTH  64             /* 유저 이름 최대 길이 */


void              show_certs(UserContext *user, SSL* ssl);

// 메시지 리시버 연결 요청 쓰레드에 정치 요청 여부를 반환합니다.
bool              is_broad_suspend(UserContext* user);


// 메시지 리시버 연결 요청 쓰레드를 정지시킵니다.
void              set_broad_suspend(UserContext* user);

int               get_user_allocated_index(UserContext* user);

void              set_user_board_thread(UserContext* user, pthread_t* tid);
pthread_t*        get_user_board_thread(UserContext* user);

void              set_user_broad_conn(UserContext* user, Connection* connection);
Connection*       get_user_broad_conn(UserContext* user);

ConnectManager*   get_user_connect_manager(UserContext* user);
ConnectManager*   get_user_broad_connect_manager(UserContext* user);

Connection*       get_user_conn(UserContext* user);

char*             get_user_name(UserContext* user);
void              set_user_name(UserContext* user, char* username);


// 클라이언트(유저)가 핸드쉐이크까지 정상적으로 수행했는지 여부를 반환합니다.
bool              is_user_verified(UserContext* user);
bool              set_user_verified(UserContext* user, bool verified);

void              set_user_mutex(UserContext* user, pthread_mutex_t* mutex);
pthread_mutex_t*  get_user_mutex(UserContext* user);


// 서버에 연결된 인원 수를 반환합니다.
int               get_current_established_user(ConnectManager* cm);


// 검증 핸드쉐이크까지 마친 유저 인원 수를 반환합니다.
int               get_current_joined_user(ConnectManager* cm);


// 유저가 서버와의 핸드쉐이크를 마친 상태인지 확인합니다.
// 클라이언트 검증 여부는 판단하지 않습니다. 
bool              is_user_established_server(UserContext* user);


// 유저가 서버와의 핸드쉐이크를 모두 마친 후, 정상적으로 접속중인지
// 확인합니다.
bool              is_user_joined_server(UserContext* user);


// 유저 데이터를 통해 클라이언트의 정보를 읽어들어 서버와의 연결을 끊습니다.
// prevent_shutdown은 SSL_Shutdown에 대한 호출 여부를 결정합니다.
bool              user_disconnect(UserContext* user, bool prevent_shutdown);

int               user_broad_disconnect(UserContext* user);

/**
 * @brief 유저 데이터에 대한 할당을 헤제합니다.
 * 
 * @param user 유저 컨텍스트 값을 포함합니다.
 * @return int 유저 데이터에 대한 할당이 성공적으로 헤제되면 0을 반환하고, 
 *         그렇지 않다면 -1을 반환합니다. 
 */
int               user_free(UserContext* user);

///
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
///
/// ConnectManager 대한 getter, setter, 응용 기능에 대한 함수를 정의합니다.
///
int               create_broad_session_job(ConnectManager* server_ctx, char* host_ip, int port);

// 클라이언트와 서버를 연결시켜 줄 수 있는 ConnectManger 인스턴스를 생성합니다.
ConnectManager*   connect_manager_create(bool isInitialized);

// ConnectManger 인스턴스를 실행시킵니다.
bool              connect_manager_execute(ConnectManager* ctx, char* host_ip, int port);

UserContext**            get_connect_manager_user(ConnectManager* cm);
Connection*       get_connect_manager_of_conn(ConnectManager* ctx);
pthread_mutex_t*  get_connect_manager_of_mutex(ConnectManager* ctx);
pthread_mutex_t*  get_connect_manager_of_message_mutex(ConnectManager* ctx);

// ConnectManager에 대한 모든 자원을 헤제합니다.
// SSL_CTX와 관련된 자원은 헤제하지 않습니다.
void              connect_manager_free(ConnectManager* ctx);

///
//////////////////////////////////////////////////////////////////

#endif