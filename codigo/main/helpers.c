#include "helpers.h"

bool is_number(char* str, int len)
{
    len = len == 0 ? strlen(str) : len;
    if (len == 0) {
        return false;
    }

    for (int i=0; i<len; i++) {
        if (str[i] > '9' || str[i] < '0') {
            return false;
        }
    }

    return true;
}
