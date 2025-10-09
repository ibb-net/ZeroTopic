
#include <windows.h>
#include "os_init.h"

ssize_t os_init(void)
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0) {
        return -1;
    }

    return 0;
}

ssize_t os_exit(void)
{
    if (WSACleanup() != 0) {
        return -1;
    }

    return 0;
}