/**
 * @file server_broad.h
 * @author ruskonert
 * @brief 클라이언트의 메시지 수신기를 대상으로 연결을 수행하는 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-23
 */

#ifndef SERVER_BRORD_H
#define SERVER_BROAD_H

#include "../connection.h"
#include "../comm.h"
#include "../util.h"

#include "server_net.h"

/**
 * 클라이언트의 메시지 수신기에게 메세지를 전송합니다.
 * 
 * @param argv 포인터 배열이 포함됩니다. 0번째는 유저 정보, 1번째는 스케줄 메시지가 전달됩니다.
 * @return void* 클라이언트의 Response code가 반환됩니다.
 */
void* broad_send_message(void* argv);


/**
 * 클라이언트의 메시지 수신기에 연결을 시도합니다.
 * 
 * @param argv 유저 정보가 포함됩니다.
 * @return void* 연결에 성공하면 0을 반환하고, 그렇지 않다면 -1을 반환합니다.
 */
void* broad_connect_session(void* argv);


#endif