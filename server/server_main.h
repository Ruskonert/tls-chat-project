/**
 * @file server_main.h
 * @author ruskonert
 * @brief 서버 프로그램의 main 함수를 구현합니다.
 * @version 1.0
 * @date 2022-02-18
 * 
 */

#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

#include "../comm.h"


// 서버에서 사용 가능한 커맨드 함수들을 가져옵니다.
COMMAND_FUNC* get_prog_command_function();


#endif