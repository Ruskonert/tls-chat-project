/**
 * @file util.h
 * @author ruskonert
 * @brief 소스 코드가 동작하는 데 유용한 보조적 기능을 구현합니다.
 * @version 1.0
 * @date 2022-02-14
 */

#ifndef UTIL_H
#define UTIL_H

#include "comm.h"

/**
 * @brief 문자열 내 일부 포지션 지점에 있는 문자열 일부를 가져옵니다.
 * 
 * @param string 대상 문자열을 지정합니다. 
 * @param position 대상 문자열에서 특정 문자열을 읽기 위한 인덱스 기준을 설정합니다.
 * @param length 시작점으로 떨어진 길이입니다.
 * @return char* 시작 포지션을 기준으로 하여 지정한 길이만큼 해당하는 문자열 값을 반환합니다.
 */
char *substring(char *string, int position, int length);

/**
 * @brief 콘솔 윈도우에 유저 정보를 담아 메시지를 출력합니다.
 * 
 * @param type 메시지 타입을 지정합니다.
 * @param conn 유저 컨텍스트를 지정합니다. null이면 시스템 메시지로 간주합니다.
 * @param lock 뮤텍스를 지정합니다. null이면 메시지 출력시 쓰레드 안전성을 고려하지 않습니다.
 * @param format 메시지 출력을 위한 형식 문자열을 포함합니다.
 * @param ... 포맷 문자열 인자를 포함합니다.
 */
void output_message(MESSAGE_TYPE type, Connection* conn, pthread_mutex_t* lock, const char* format, ...);

#endif