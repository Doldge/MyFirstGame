#include "serializer.h"

unsigned char * serialize_int(unsigned char * buffer, int input )
{
    buffer[0] = input >> 24;
    buffer[1] = input >> 16;
    buffer[2] = input >> 8;
    buffer[3] = input;

    return buffer + 4;
};
