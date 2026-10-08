/*
 * ЛР №9, базовый вариант (Windows), программа 2.
 *
 *  1. Открывает уже существующую разделяемую память и семафор, созданные программой 1,
 *     выводит адрес, по которому память отображена в ЭТОМ процессе.
 *  2. Сразу читает текст из разделяемой памяти и выводит его (с примечанием).
 *  3. Ждёт семафор (WaitForSingleObject) — сигнал о готовности текста.
 *  4. После сигнала снова читает память и выводит содержимое (с примечанием).
 *
 * Запускать ПОСЛЕ lab9_a.exe.
 * Сборка:  gcc lab9_b.c -o lab9_b.exe -Wall
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#define SHM_NAME "OSLAB9_SHM"
#define SEM_NAME "OSLAB9_SEM"
#define SHM_SIZE 4096

static void say(const char *fmt, ...)
{
    char text[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    SYSTEMTIME t;
    GetLocalTime(&t);
    printf("[%02d:%02d:%02d] %s\n", t.wHour, t.wMinute, t.wSecond, text);
    fflush(stdout);
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    say("Программа 2: открываю разделяемую память и семафор.");

    /* Открываем существующие объекты по именам (создаёт их программа 1). */
    HANDLE hMap = OpenFileMapping(FILE_MAP_READ, FALSE, SHM_NAME);
    if (!hMap) {
        say("Не удалось открыть разделяемую память (ошибка %lu). Сначала запустите lab9_a.exe.",
            GetLastError());
        return 1;
    }
    const char *pb = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, SHM_SIZE);
    if (!pb) { say("MapViewOfFile: ошибка %lu", GetLastError()); return 1; }
    say("Разделяемая память отображена в моё адресное пространство по адресу %p.", (const void *)pb);

    /* SYNCHRONIZE — право ждать семафор. */
    HANDLE hSem = OpenSemaphore(SYNCHRONIZE, FALSE, SEM_NAME);
    if (!hSem) { say("OpenSemaphore: ошибка %lu", GetLastError()); return 1; }

    /* Чтение ДО сигнала семафора: там пока только заглушка первой программы. */
    say("Читаю разделяемую память до сигнала семафора: \"%s\"", pb);

    say("Жду сигнала семафора...");
    WaitForSingleObject(hSem, INFINITE);       /* блокируемся, пока значение семафора 0 */
    say("Семафор сработал.");

    say("Читаю разделяемую память после сигнала: \"%s\"", pb);

    UnmapViewOfFile(pb);
    CloseHandle(hSem);
    CloseHandle(hMap);
    say("Программа 2: конец работы.");
    return 0;
}
