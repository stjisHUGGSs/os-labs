/*
 * ЛР №6: общие мелочи для всех программ (вывод одной строки с отметкой
 * времени и номером процесса). Подключается директивой #include "../common.h".
 */
#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

/* Консоль в UTF-8, чтобы русский текст из исходников выводился правильно. */
static inline void init_console(void)
{
    SetConsoleOutputCP(CP_UTF8);
}

/* Одна строка вывода: [время] PID | текст. Сразу сбрасываем буфер (fflush),
 * чтобы строки разных процессов, пишущих в одну консоль, не склеивались. */
static inline void say(const char *fmt, ...)
{
    char text[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    SYSTEMTIME t;
    GetLocalTime(&t);
    printf("[%02d:%02d:%02d] PID %-5lu | %s\n",
           t.wHour, t.wMinute, t.wSecond, GetCurrentProcessId(), text);
    fflush(stdout);
}

/* Запуск программы командной строкой cmd. Процесс использует консоль родителя
 * (общее окно). suspended = TRUE — создать в приостановленном состоянии
 * (нужно, чтобы успеть включить процесс в Job до начала его работы). */
static inline BOOL start_process(const char *cmd, BOOL suspended, PROCESS_INFORMATION *pi)
{
    char line[256];                          /* CreateProcess может менять строку */
    snprintf(line, sizeof(line), "%s", cmd);
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);                      /* обязательно задать размер структуры */
    DWORD flags = NORMAL_PRIORITY_CLASS | (suspended ? CREATE_SUSPENDED : 0);
    return CreateProcess(NULL, line, NULL, NULL, FALSE, flags, NULL, NULL, &si, pi);
}

#endif
