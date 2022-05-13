#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

int safewrite(int fd, char* buf, int count) {
    char buffer[1024] = {0};
    char* insert = &buffer[0];
    char* ret = buf;
    while(1) {
        const char* p = strstr(ret, "evolution");
        if (p == NULL) {
            strcpy(insert, ret);
            break;
        }
        memcpy(insert, ret, p - ret);
        insert += p - ret;
        memcpy(insert, "GOD", 3);
        insert += 3;
        ret = p + 9;
        count -= 6;
    }
    strcpy(buf, buffer);
    return write(fd, buf, count);
}