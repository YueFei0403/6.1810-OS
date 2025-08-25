#include "kernel/types.h"

// strncmp: compare up to n chars
int strncmp(const char *p, const char *q, uint n) {
    while (n > 0 && *p && (*p == *q)) {
        p++; q++; n--;
    }
    if (n == 0) return 0;
    return (uchar)*p - (uchar)*q;
}

// strncpy: copy up to n chars; pad with '\0' if src shorter
char *strncpy(char *dst, const char *src, int n) {
    char *ret = dst;
    while (n-- > 0) {
        if (*src) *dst++ = *src++;
        else { *dst++ = 0; } // pad 
    }
    return ret;
}