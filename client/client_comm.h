#ifndef CLIENT_COMM_H
#define CLIENT_COMM_H

#include "comm.h"

struct command_type_t;

Message* msg_packing_message(char* buf);

#endif