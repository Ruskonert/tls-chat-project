#include <string.h>

#include "../cmd.h"
#include "../comm.h"
#include "client_comm.h"

struct command_type_t
{
    int type;
    bool hasArgument;
};



struct command_type_t get_command_type(char* buf)
{
    struct command_type_t ct = {255, false};

    if( strcmp("/name", buf) == 0) {
        ct.type = CMD_CHANGE_NICKNAME;
        ct.hasArgument = true;
    }

    if( strcmp("/send", buf) == 0) {
        ct.type = CMD_SECRET_MESSAGE;
        ct.hasArgument = true;
    }

    if( strcmp("/status", buf) == 0) {
        ct.type = CMD_STATUS;
        ct.hasArgument = false;
    }

    if (strcmp("/exit", buf) == 0) {
        ct.type = CMD_DISCONNECT;
        ct.hasArgument = false;
    }
    return ct;
}



Message* msg_packing_message(char* buf)
{
    Message* message = -1;

    if(buf[0] == '/') {
        char* ptr = strtok(buf, " ");
        struct command_type_t ct = get_command_type(ptr);
        if(ct.type == -1) return message;
        if(ct.type == 255)
        {
            message = packing_message_create(ct.type, strlen(buf), buf);
        }
        else
        {
            char* str_message;

            if(ct.hasArgument) str_message = buf + strlen(ptr) + 1;
            else str_message = "";

            message = packing_message_create(ct.type, strlen(str_message), str_message);
        }
    }

    else {
        message = packing_message_create(CMD_BROADCASE_MESSAGE, strlen(buf), buf);
    }
    return message;
}