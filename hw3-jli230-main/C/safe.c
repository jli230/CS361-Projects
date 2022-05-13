#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

extern ssize_t write(int fd, char* buf, int count) {
    char buffer[1024] = {0};
    char* insert = &buffer[0];
    char* ret = buf;
    while(1) {
        char* p = strstr(ret, "evolution");
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
    char* newstr = malloc(strlen(buffer));
    strcpy(newstr, buffer);
    size_t(*real_write)(int, const void*, size_t) = dlsym(RTLD_NEXT, "write");
    real_write(fd, newstr, count);
    exit(1);
}