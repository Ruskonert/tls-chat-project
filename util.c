/**
 * @file util.c
 * @author ruskonert
 * @brief 소스 코드가 동작하는 데 유용한 보조적 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-14
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "connection.h"
#include "util.h"



char *substring(char *string, int position, int length) 
{
   char *pointer = malloc(length + 1);
   int c;

   memset(pointer, 0, length + 1);

   if (pointer == NULL)
   {
      output_message(MSG_ERROR, NULL, NULL, "[%s] 메모리를 할당할 수 없음\n", __FUNCTION__);
      exit(1);
   }

   for (c = 0 ; c < length ; c++)
   {
      *(pointer + c) = *(string + position - 1);      
      string++;   
   }
   return pointer;
}



void output_message(MESSAGE_TYPE type, Connection* conn, pthread_mutex_t* lock, const char* format, ...)
{
    time_t mytime = time(NULL);
    char * time_str = ctime(&mytime);
    const char* user_addr;

    time_str[strlen(time_str)-1] = '\0';

    // 유저 정보에 대한 문자열을 할당합니다.
    if(conn == 0) {
        user_addr = "SYSTEM";
    }
    else {
        user_addr = get_conn_ip_addr(conn);
    }

    va_list ap;
    char buf[4096] = {0,};
    
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    const char* message_type;

    // 메시지 유형에 따른 접두사를 할당합니다.
    switch(type) {
        case MSG_CONNECTION: 
            message_type = "CONN";
            break;
        case MSG_INFO:
            message_type = "INFO";
            break;
        case MSG_MESSAGE:
            message_type = "MSG";
            break;
        case MSG_WARNING:
            message_type = "WARN";
            break;
        case MSG_ERROR: 
            message_type = "ERR";
            break;
        case MSG_COMMAND:
            message_type = "CMD";
            break;
        default:
            message_type = "UNKNOWN";
    }

    // 락을 걸어 메시지 출력에 대한 Safety를 보장시킵니다.
    if( lock != 0 ) {
        pthread_mutex_lock(lock);
        fprintf(stdout, "[%s] %s / [%s] %s", time_str, user_addr, message_type, buf);
        pthread_mutex_unlock(lock);
    }

    else {
        fprintf(stdout, "[%s] %s / [%s] %s", time_str, user_addr, message_type, buf);
    }
}