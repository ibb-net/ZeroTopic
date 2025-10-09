
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include "os_printf.h"

ssize_t os_printf(const char* pFormat, ...)
{
    ssize_t RetVal = 0;

    va_list args;
    va_start(args, pFormat);
    RetVal = vprintf(pFormat, args);
    va_end(args);

    return RetVal;
}

ssize_t os_console_key_get(void)
{
    WINBOOL RetVal = 0;
    DWORD   Mode = 0;
    HANDLE  Handle = GetStdHandle(STD_INPUT_HANDLE);

    DWORD            Counter;
    INPUT_RECORD     Records;

    if (Handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    RetVal = ReadConsoleInput(Handle, &Records, 1, &Counter);
    if (RetVal < 0) {
        return -2;
    }

    if ((Counter > 0) &&
        (Records.EventType == KEY_EVENT) &&
        (Records.Event.KeyEvent.bKeyDown > 0)) {

        return Records.Event.KeyEvent.wVirtualKeyCode;
    } else {
        return 0;
    }
}

ssize_t os_console_echo_set(OsConsoleBool_e En)
{
    WINBOOL RetVal = 0;
    DWORD   Mode = 0;
    HANDLE  Handle = GetStdHandle(STD_INPUT_HANDLE);

    if (Handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    RetVal = GetConsoleMode(Handle, &Mode);
    if (RetVal < 0) {
        return -2;
    }

    if (En == OS_CONSOLE_TRUE) {
        Mode = Mode  | ENABLE_ECHO_INPUT;
    } else {
        Mode = Mode  & (~ENABLE_ECHO_INPUT);
    }

    RetVal = SetConsoleMode(Handle, Mode);
    if (RetVal < 0) {
        return -3;
    }

    return 0;
}

ssize_t os_console_ansi_set(OsConsoleBool_e En)
{
    WINBOOL RetVal = 0;
    DWORD   Mode = 0;
    HANDLE  Handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (Handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    RetVal = GetConsoleMode(Handle, &Mode);
    if (RetVal < 0) {
        return -2;
    }

    if (En == OS_CONSOLE_TRUE) {
        Mode = Mode  | ENABLE_VIRTUAL_TERMINAL_INPUT;
    } else {
        Mode = Mode  & (~ENABLE_VIRTUAL_TERMINAL_INPUT);
    }

    RetVal = SetConsoleMode(Handle, Mode);
    if (RetVal < 0) {
        return -3;
    }

    return 0;
}
