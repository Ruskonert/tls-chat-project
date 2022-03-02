#include <stdio.h>

#include "comm.h"
#include "connection.h"

#define TEST_STR "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA \
                AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" 

int main(int argc, char* argv[])
{
    char buf[1024] = {0, };

    strcpy(buf, TEST_STR);

    int len = strlen(buf);

    Message* message = packing_message_create(2, len, buf);
    printf("%s\n", packing_message_convert_string(message));

    return 0;
}