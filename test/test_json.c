#include <stdio.h>
#include <json.h>

#include "../comm.h"
 
int main(int argc, char **argv)
{
    json_object *myobj, *dataobj;
    const char* str = "\x88\x99HelloWorld";

    Message* message = packing_message_create(0xFE, strlen(str), (char*)str);
    json_object* message_jobj = packing_message_json(message);

    printf("Packing Message: \n%s\n", json_object_to_json_string(message_jobj));
    json_object_put(myobj);

    return 0;
}