/**
 * @file cmd.h
 * @author ruskonert
 * @brief 커맨드 유형을 정의합니다.
 * @version 0.1
 * @date 2022-02-23
 */

#ifndef CMD_HEADER_H
#define CMD_HEADER_H

#define CMD_HANDSHAKE              0x00
#define CMD_BROADCASE_MESSAGE      0x01
#define CMD_CHANGE_NICKNAME        0x02
#define CMD_SECRET_MESSAGE         0x03
#define CMD_STATUS                 0x04
#define CMD_DISCONNECT             0x05

#define CMD_NOT_SUPPORTED          0xFF

typedef struct command_t
{
    char    command[128];
    int     arg_count;
    char**  argv;
} Command;

Command* convert_arguments(char* str);

#endif