/**
 * @file server_net.h
 * @author ruskonert
 * @brief TLS 핸드쉐이크 과정을 구현하고, 클라이언트 연결 정보를 관리하기 위한 기능, 
 *        구조체를 정의합니다.
 * @version 1.0
 * @date 2022-02-17
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/opensslconf.h>

#if !defined(OPENSSL_THREADS)
# error "This OpenSSL version is not supported thread"
#endif

#include "server_net.h"

#include "../comm.h"
#include "../connection.h"
#include "../util.h"

extern void* communicate_broad_user(void* argv);
extern void* communicate_user(void* argv);

// 클라이언트 연결 정보 및 유저의 기본 속성을 정의합니다.
struct user_t 
{
    int                        _alloc;                      /* 연결 매니저가 할당한 인덱스 번호 */
    char                       uname[USERNAME_MAX_LENGTH];  /* 유저 이름 */
    Connection*                conn;
    ConnectManager*            cm_ctx;
    pthread_t*                 _tid;
    pthread_mutex_t*           _mutex;
    bool                       verified;                    /* 클라이언트 검증 완료 유무 */

    ConnectManager*            broad_cm_ctx;
    bool                       _broad_suspend;
    pthread_t*                 _broad_tid;                  /* 클라이언트의 메시지 수신기 연결 쓰레드 */
    Connection*                _broad_conn;                 /* 클라이언트의 메시지 수신기 연결 정보 */
};


// 클라이언트와 연결을 수행하기 위한 부속 정보를 정의합니다.
struct connect_manager_t
{
    bool                  isInitialized;
    bool                  isSuspend;                        /* 동작 중지 유무 */

    Connection*           _server_attr_ctx;                 /* 서버 연결 정보 */
    UserContext**         _user_arr;                        /* 서버에 접속 중인 유저 리스트 */
    pthread_t*            _tid;
    pthread_mutex_t*      _mutex;
    pthread_mutex_t*      _message_mutex;

};


/**
 * @brief 클라이언트의 인증서 정보를 확인합니다.
 * 
 * @param user 유저 컨텍스트 값이 포함됩니다.
 * @param ssl 유저의 SSL 컨텍스트가 전달됩니다.
 */
void show_certs(UserContext *user, SSL* ssl)
{   X509 *cert;
    char *line;

    Connection* conn = get_user_conn(user);
    pthread_mutex_t* mutex = get_connect_manager_of_mutex(get_user_connect_manager(user));
    
    cert = SSL_get_peer_certificate(ssl);
    if ( cert != NULL ) {

        output_message(MSG_INFO, conn, mutex, "Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        output_message(MSG_INFO, conn, mutex, "Subject: %s\n", line);

        free(line);

        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        output_message(MSG_INFO, conn, mutex, "Issuer: %s\n", line);
        free(line);
        
        X509_free(cert);
    }
    else {
        output_message(MSG_INFO, conn, mutex, "No client certificates configured.\n");
    }
}


ConnectManager* get_user_broad_connect_manager(UserContext* user)
{
    return user->broad_cm_ctx;
}



void set_user_board_thread(UserContext* user, pthread_t* tid)
{
    user->_broad_tid = tid;
}



pthread_t* get_user_board_thread(UserContext* user)
{
    return user->_broad_tid;    
}


pthread_mutex_t*  get_user_mutex(UserContext* user)
{
    return user->_mutex;
}


void set_user_broad_conn(UserContext* user, Connection* connection)
{
    user->_broad_conn = connection;
}



Connection* get_user_broad_conn(UserContext* user)
{
    return user->_broad_conn;    
}



bool is_user_verified(UserContext* user)
{
    return user->verified;
}



int get_user_allocated_index(UserContext* user)
{
    return user->_alloc;
}



bool set_user_verified(UserContext* user, bool verified)
{
    return user->verified = verified;
}



Connection* get_user_conn(UserContext* user)
{
    return user->conn;
}



Connection* get_connect_manager_of_conn(ConnectManager* ctx)
{
    return ctx->_server_attr_ctx;
}



ConnectManager* get_user_connect_manager(UserContext* user) { return user->cm_ctx; }



char* get_user_name(UserContext* user) { return user->uname; }


pthread_mutex_t* get_connect_manager_of_mutex(ConnectManager* ctx) { return ctx->_mutex; }


pthread_mutex_t* get_connect_manager_of_message_mutex(ConnectManager* ctx)
{
    return ctx->_message_mutex;
}



UserContext** get_connect_manager_user(ConnectManager* cm) {  return cm->_user_arr; }



void set_user_name(UserContext* user, char* username)
{
    if ( strlen(username) > USERNAME_MAX_LENGTH ) return;
    strcpy(user->uname, username);
}



bool is_user_established_server(UserContext* user)
{
    if(user == 0) return false;
    if(user->conn == 0) return false;
    return get_conn_status(user->conn) == USER_STATUS_CONNECTION_ESTATISHED;
}



bool is_user_joined_server(UserContext* user) 
{
    if(user == 0) return false;
    if(user->conn == 0) return false;
    return get_conn_status(user->conn) >= USER_STATUS_JOINED_SERVER;
}



int get_current_established_user(ConnectManager* cm)
{
    int idx = 0;
    int count = 0;
    while( idx < MAX_USER_CONNECTION) {
        UserContext* user = cm->_user_arr[idx];
        if(user != 0 && is_user_established_server(user))
        {
            count++;
        }
        idx++;
    }

    return count;
}



int get_current_joined_user(ConnectManager* cm)
{
    int idx = 0;
    int count = 0;
    while( idx < MAX_USER_CONNECTION ) {
        UserContext* user = cm->_user_arr[idx];
        if(user != 0 && is_user_joined_server(user))
        {
            count++;
        }
        idx++;
    }

    return count;
}



int get_current_broad_user(ConnectManager* cm)
{
    int idx = 0;
    int count = 0;
    while( idx < MAX_USER_CONNECTION ) {
        UserContext* user = cm->_user_arr[idx];
        if(user != 0 && get_user_broad_conn(user) == 0)
        {
            count++;
        }
        idx++;
    }

    return count;
}



UserContext* start_communicate_user(ConnectManager* ctx, SOCKET_HANDLE user_sd, struct sockaddr* addr)
{
    SSLContext* server_ssl_ctx = get_conn_ssl_context(ctx->_server_attr_ctx);
    SSL* ssl_user_sd = SSL_new(get_ssl_ctx_context(server_ssl_ctx));

    SSL_set_fd(ssl_user_sd, user_sd);
    SSL_set_accept_state(ssl_user_sd);

    if ( SSL_accept(ssl_user_sd) == -1 ) {
        ERR_print_errors_fp(stderr);
        return (UserContext*)-1;
    }
    Connection* conn = connection_create(server_ssl_ctx);
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));

    pthread_mutex_init(mutex, NULL);

    UserContext* user = (UserContext*)malloc(sizeof(UserContext));

    user->_mutex = mutex;
    user->_tid = thread;
    user->cm_ctx = ctx;
    user->conn = conn;
    user->_broad_tid = 0;
    user->_broad_conn = 0;
    user->_broad_suspend = false;

    set_conn_ssl_session(conn, ssl_user_sd);
    set_conn_status(conn, USER_STATUS_CONNECTION_ESTATISHED);
    set_conn_ip_addr(conn, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr));

    if( __builtin_expect( pthread_create(thread, NULL, communicate_user, (void*)user) == -1, 0)) {
        printf("Generating thread was failed!\n");
        return (UserContext*)-1;   
    }
    return user;
}


/**
 * 할당 가능한 유저 컨텍스트의 인덱스를 반환합니다.
 */
int current_user_index(ConnectManager* ctx)
{
    pthread_mutex_lock(ctx->_mutex);
    int idx = 0;
    
    while( idx < MAX_USER_CONNECTION + 64) {
        UserContext* user = ctx->_user_arr[idx];
        if(user == 0) {
            pthread_mutex_unlock(ctx->_mutex);
            return idx;
        }
        idx++;
    }
    pthread_mutex_unlock(ctx->_mutex);
    return -1;
}



bool is_broad_suspend(UserContext* user)
{
    return user->_broad_suspend;
}



void set_broad_suspend(UserContext* user)
{
    if(user == 0) return;
    user->_broad_suspend = !user->_broad_suspend;
}



bool user_disconnect(UserContext* user, bool safety_shutdown) 
{
    if( user == 0 ) return false; 
    
    Connection* conn = user->conn;
    ConnectManager* cm = get_user_connect_manager(user); 

    pthread_mutex_t* mutex = get_connect_manager_of_mutex(cm);
    pthread_mutex_t* message_mutex = get_connect_manager_of_message_mutex(cm);

    int status = get_conn_status(conn);

    if(status == USER_STATUS_DISCONNECTED || status == USER_STATUS_NOT_ESTABLISHED) {
        return false;
    }

    disconnect(conn, mutex, safety_shutdown);
    output_message(MSG_CONNECTION, conn, message_mutex, "Disconnected chat server\n");
    user_free(user);

    return true;
}


int user_broad_disconnect(UserContext* user)
{
    ConnectManager *cm_ctx = get_user_broad_connect_manager(user);

    if(user->_broad_tid != 0) {
        pthread_mutex_t* mutex = get_connect_manager_of_mutex(cm_ctx);
        if(user->_broad_conn != 0) {

            pthread_mutex_lock(mutex);

            set_broad_suspend(user);
            pthread_mutex_unlock(user->_mutex);

            pthread_mutex_unlock(mutex);

            disconnect(user->_broad_conn, mutex, true);
            
            output_message(MSG_CONNECTION, user->_broad_conn, user->cm_ctx->_mutex, "Broadcast client was disconnected by chat server\n");

            conn_ctx_free(user->_broad_conn);
            set_user_broad_conn(user, 0);
        }

        pthread_mutex_lock(mutex);

        // 클라이언트의 메세지 수신기 연결을 대기하고 있는 쓰레드 작업을 종료합니다.
        pthread_kill(user->_broad_tid, SIGINT);
        free(user->_broad_tid);
        set_user_board_thread(user, 0);

        pthread_mutex_unlock(mutex);
        free(user->_mutex); 
        return 0;
    }
    else {
        return -1;
    }
}


int user_free(UserContext* user)
{
    if(user == 0) return -1;

    if(get_conn_status(user->conn) == USER_STATUS_DISCONNECTED)
    {
        ConnectManager *cm_ctx = get_user_connect_manager(user);

        int alloc = user->_alloc;
        
        conn_ctx_free(user->conn);       
        free(user->_tid);
        free(user);
        
        cm_ctx->_user_arr[alloc] = 0;
        
        return 0;
    }
    return -1;
}

/**
 * 
 * 
 * @param user 
 * @param user_sd 
 * @param addr 
 */
void start_communicate_broadcast_user(UserContext* user, SOCKET_HANDLE user_sd, struct sockaddr* addr)
{
    ConnectManager* server_cm_ctx = get_user_connect_manager(user);
    SSLContext* server_broad_ssl_ctx = get_conn_ssl_context(server_cm_ctx->_server_attr_ctx);
    SSL* ssl_user_sd = SSL_new(get_ssl_ctx_context(server_broad_ssl_ctx));

    char* ip = inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);

    SSL_set_fd(ssl_user_sd, user_sd);
    SSL_set_accept_state(ssl_user_sd);

    if ( SSL_accept(ssl_user_sd) == -1 ) {
        ERR_print_errors_fp(stderr);
        output_message(MSG_ERROR, NULL, get_connect_manager_of_message_mutex(server_cm_ctx), "Failed to accept the ssl session, from: %s\n", ip);
        return;
    }

    Connection* broad_conn = connection_create(server_broad_ssl_ctx);
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));

    pthread_mutex_init(mutex, NULL);

    set_conn_ssl_session(broad_conn, ssl_user_sd);
    set_conn_status(broad_conn, USER_STATUS_CONNECTION_ESTATISHED);
    set_conn_ip_addr(broad_conn, ip);

    user->_broad_conn = broad_conn;
    user->_broad_tid = thread;
    user->_broad_suspend = false;

    if( __builtin_expect( pthread_create(thread, NULL, communicate_broad_user, (void*)user) == -1, 0)) {
        printf("Generating thread was failed!\n");
    }
}

/**
 *
 * 
 * @param server_ctx 
 * @param host_ip 
 * @param port 
 * @return int 
 */
int create_broad_session_job (ConnectManager* server_ctx, char* host_ip, int port)
{
    ConnectManager* broad_cm_ctx = connect_manager_create(false);
    Connection* server_broad_conn = get_connect_manager_of_conn(broad_cm_ctx);

    pthread_mutex_t* broad_conn_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(broad_conn_mutex, NULL);
    
    broad_cm_ctx->_message_mutex = server_ctx->_message_mutex;
    broad_cm_ctx->_mutex = broad_conn_mutex;
    broad_cm_ctx->_user_arr = server_ctx->_user_arr;

    SOCKET_HANDLE sd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0, };

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host_ip);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( __builtin_expect(bind(sd, (struct sockaddr*)&addr, sizeof(addr) ) != 0, 0)) {
        output_message(MSG_ERROR, NULL, broad_cm_ctx->_message_mutex, "binding of broadcast server address was failed!\n");
        return (void*)-1;
    }

    if ( __builtin_expect(listen(sd, 5) != 0, 0) ) {
        output_message(MSG_ERROR, NULL, broad_cm_ctx->_message_mutex, "listening of broadcast server address was failed!\n");
        return (void*)-1;
    }

    output_message(MSG_INFO, NULL, broad_cm_ctx->_message_mutex, "Listening address for broadcasting client -> %s:%d\n", host_ip, port);

    set_conn_socket_id(server_broad_conn, sd);
    set_conn_ip_addr(server_broad_conn, host_ip);

    while (!broad_cm_ctx->isSuspend) {
        struct sockaddr_in user_addr = {0, };
        socklen_t len = sizeof(user_addr);

        SOCKET_HANDLE user_sd = accept(sd, (struct sockaddr*)&user_addr, &len);
        const char* user_ip = inet_ntoa(((struct sockaddr_in *)&user_addr)->sin_addr);
        output_message(MSG_CONNECTION, NULL, broad_cm_ctx->_message_mutex, "Tried to connect the broadcast channel from %s\n", user_ip);

        UserContext* user = -1;

        // 연결 요청한 클라이언트가 채팅 서버에 접속한 상태인지 검증합니다.
        for(int i = 0; i < MAX_USER_CONNECTION; i++) {
            UserContext* o_user = broad_cm_ctx->_user_arr[i];
            if(is_user_joined_server(o_user)) {
                Connection* user_conn = get_user_conn(o_user);
                const char* ip = get_conn_ip_addr(user_conn);

                // IP가 일치하면, 현재 클라이언트가 연결되어 있음을 의미합니다.
                if(strcmp(ip, user_ip) == 0) {
                    user = o_user;
                    user->broad_cm_ctx = broad_cm_ctx;
                    start_communicate_broadcast_user(user, user_sd, (struct sockaddr*)&user_addr);
                    break;
                }
            }
        }
        if (user == (UserContext*)-1) {
            output_message(MSG_CONNECTION, NULL, broad_cm_ctx->_message_mutex, "Invaild requested, closed connection for broadcasting -> %s:%d\n", user_ip, port);
            close(user_sd);
        }
    }
}


// TCP 프로토콜에 기반하여 새로운 유저 연결에 대해서 대기하고 처리합니다.
void* wait_for_new_user(void* argv) 
{
    void** argv_ptr = (void**)argv;

    ConnectManager* ctx = (ConnectManager*)argv_ptr[0];
    const char* host_ip = (const char*)argv_ptr[1]; 
    int port = *(int*)argv_ptr[2];
    Connection* server_conn = get_connect_manager_of_conn(ctx);

    SOCKET_HANDLE sd, user_sd;
    struct sockaddr_in addr = {0, };

    free(argv);

    sd = socket(PF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host_ip);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( __builtin_expect(bind(sd, (struct sockaddr*)&addr, sizeof(addr) ) != 0, 0)) {
        output_message(MSG_ERROR, NULL, ctx->_message_mutex, "binding of server address was failed!\n");
        return (void*)-1;
    }

    if ( __builtin_expect(listen(sd, 5) != 0, 0) ) {
        output_message(MSG_ERROR, NULL, ctx->_message_mutex, "listening of server address was failed!\n");
        return (void*)-1;
    }

    output_message(MSG_INFO, NULL, ctx->_message_mutex, "Listening address -> %s:%d\n", host_ip, port);

    set_conn_socket_id(server_conn, sd);
    set_conn_ip_addr(server_conn, host_ip);

    while( !ctx->isSuspend ) {
        struct sockaddr_in c_addr = {0, };
        socklen_t len = sizeof(c_addr);
        UserContext* user;

        // 새로운 유저가 연결될때마다 처리합니다.
        user_sd = accept(sd, (struct sockaddr*)&c_addr, &len);
        int idx = current_user_index(ctx);

        if(idx == -1) {
            output_message(MSG_ERROR, NULL, ctx->_message_mutex, "Failed to allocate user, because user index is invaild\n");
            close(user_sd);
            continue;
        }

        if( __builtin_expect(user_sd < 0, 0) ) {
            output_message(MSG_ERROR, NULL, ctx->_message_mutex, "Communication failed, except the socket\n");
            continue;
        }

        int current_count = get_current_established_user(ctx);
        if(current_count > MAX_USER_CONNECTION + 64) {
            output_message(MSG_ERROR, NULL, ctx->_message_mutex, "Server is busy, ignoring connection\n");
            close(user_sd);
            continue;
        }

        // 접속한 클라이언트 데이터에 기반하여, 서버에서 관리하기 위한 유저 컨텍스트 정보를 생성합니다.
        user = start_communicate_user(ctx, user_sd, (struct sockaddr*)&c_addr);
        if(user == (UserContext*)-1) {
            output_message(MSG_ERROR, NULL, ctx->_message_mutex, "Some of client was connected, but SSL service is invaild\n");
            close(user_sd);
            continue;
        }

        user->_alloc = idx;

        // 랜덤 닉네임 생성
        char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";        
        char randomString[12];

        if (randomString) {            
            for (int n = 0; n < 12; n++) {            
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }
            randomString[12] = '\0';
        }

        set_user_name(user, randomString);

        if( __builtin_expect(user == (UserContext*)-1, 0)) {
            output_message(MSG_ERROR, NULL, ctx->_message_mutex, "Failed to allocate the user, except the user\n");
            close(user_sd);
            continue;
        }
        else {
            ctx->_user_arr[idx] = user;
        }
    }
}



bool connect_manager_execute(ConnectManager* ctx, char* host_ip, int port)
{
    pthread_t* thread;
    pthread_mutex_t* mutex, *message_mutex;

    int* port_ptr = (int*)malloc(sizeof(int));
    void** argv;

    if( __builtin_expect(!ctx->isInitialized, 1) ) {
        printf("You need to initialize the context at first.\n");
        return false;
    }

    if ( __builtin_expect(ctx->_mutex != 0, 0) ) {
        printf("The connect manager was already running.\n");
        return false;
    }

    mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    message_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    thread = (pthread_t*) malloc(sizeof(pthread_t));
 
    // 뮤텍스 초기화를 수행합니다.
    if( __builtin_expect(!pthread_mutex_init(mutex, NULL) == 0, 0)) {
        printf("Initializing the mutex for connect manager was failed!\n");
        return false;
    }

    if( __builtin_expect(!pthread_mutex_init(message_mutex, NULL) == 0, 0)) {
        printf("Initializing the message mutex for connect manager was failed!\n");
        return false;
    }

    ctx->isSuspend = false;
    ctx->_message_mutex = message_mutex;
    ctx->_mutex = mutex;
    ctx->_tid = thread;
    
    // 유저에 대한 여유 할당 추가
    ctx->_user_arr = (UserContext **)malloc(sizeof(UserContext*) * ((MAX_USER_CONNECTION + 1) + 64));

    memset(ctx->_user_arr, 0, sizeof(UserContext*) * ((MAX_USER_CONNECTION + 1) + 64));

    argv = (void**)malloc(sizeof(unsigned long*) * 3);
    *port_ptr = port;

    argv[0] = ctx;
    argv[1] = host_ip;
    argv[2] = port_ptr;

    pthread_create(thread, NULL, wait_for_new_user, (void*)argv);

    output_message(MSG_INFO, NULL, ctx->_mutex, "Chat server started.\n");
    return true;    
}



ConnectManager* connect_manager_create(bool isInitialized)
{
    ConnectManager* cmanager = (ConnectManager*)malloc(sizeof(ConnectManager));
    memset(cmanager, 0, sizeof(ConnectManager));

    SSLContext* ssl_ctx = create_server_ssl_context(isInitialized);
    Connection* conn = connection_create(ssl_ctx);

    cmanager->_server_attr_ctx = conn;
    cmanager->isInitialized = true;
    cmanager->isSuspend = false;

    return cmanager;
}



void connect_manager_free(ConnectManager* ctx)
{
    free(ctx->_message_mutex);
    free(ctx->_mutex);
    conn_ctx_free(ctx->_server_attr_ctx);
    free(ctx->_user_arr);
    free(ctx->_tid);
    free(ctx);
}