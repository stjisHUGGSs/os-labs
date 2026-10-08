/*
 * ЛР №2, усложнённый вариант (Windows): то же задание, что lab2_read.c,
 * но дублирование хэндла выполняется функцией DuplicateHandle (вместо dup).
 *
 *   h1 = CreateFile(...)       — первое открытие файла;
 *   h2 = DuplicateHandle(h1)   — второй хэндл на ТОТ ЖЕ объект «файл»;
 *   h3 = CreateFile(...)       — повторное открытие: новый объект «файл».
 *
 * Позиция устанавливается через h1 на 10-й байт, затем через h1, h2, h3
 * читается по 7 символов; каждый кусок выводится в своей строке.
 *
 * Сборка:  gcc lab2_win.c -o lab2_win.exe -Wall
 * Запуск:  .\lab2_win.exe        (файл data.txt — рядом с программой)
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#define FNAME   "data.txt"
#define PORTION 7
#define START   10

static HANDLE hOut;   /* хэндл стандартного вывода */

/* Формирует строку и выводит её в stdout небуферизованным WriteFile. */
static void print(const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    DWORD written;
    WriteFile(hOut, msg, (DWORD)len, &written, NULL);
}

/* Читает PORTION байтов через хэндл h и выводит их в отдельной строке. */
static void read_and_print(const char *name, HANDLE h)
{
    char buf[PORTION + 1];
    DWORD n = 0;
    if (!ReadFile(h, buf, PORTION, &n, NULL)) {
        print("%s: ошибка ReadFile, код %lu\r\n", name, GetLastError());
        return;
    }
    buf[n] = '\0';
    print("%s: %s\r\n", name, buf);
}

/* Текущая позиция в файле (SetFilePointer с нулевым смещением от FILE_CURRENT). */
static unsigned long pos(HANDLE h)
{
    return SetFilePointer(h, 0, NULL, FILE_CURRENT);
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return 1;

    /* 1-й хэндл. FILE_SHARE_READ позволяет открывать файл повторно. */
    HANDLE h1 = CreateFile(FNAME, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h1 == INVALID_HANDLE_VALUE) {
        print("Не открыт файл " FNAME ", код %lu\r\n", GetLastError());
        return 1;
    }

    /* 2-й хэндл: копия h1 в пределах текущего процесса. Оба хэндла ссылаются
     * на один и тот же объект ядра «файл», а значит, на одну и ту же позицию. */
    HANDLE h2 = NULL;
    if (!DuplicateHandle(GetCurrentProcess(), h1,   /* откуда: процесс, хэндл */
                         GetCurrentProcess(), &h2,  /* куда: процесс, адрес копии */
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
        print("DuplicateHandle: код %lu\r\n", GetLastError());
        return 1;
    }

    /* 3-й хэндл: второе открытие того же файла — отдельный объект с позицией 0. */
    HANDLE h3 = CreateFile(FNAME, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h3 == INVALID_HANDLE_VALUE) {
        print("Не открыт файл повторно, код %lu\r\n", GetLastError());
        return 1;
    }

    /* Позиционирование через ПЕРВЫЙ хэндл: 10 байт от начала файла. */
    if (SetFilePointer(h1, START, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        print("SetFilePointer: код %lu\r\n", GetLastError());
        return 1;
    }

    print("Значения хэндлов: h1 = %p, h2 = %p, h3 = %p\r\n", h1, h2, h3);

    read_and_print("h1", h1);
    read_and_print("h2", h2);
    read_and_print("h3", h3);

    print("Текущие позиции: h1 = %lu, h2 = %lu, h3 = %lu\r\n",
          pos(h1), pos(h2), pos(h3));

    CloseHandle(h1);
    CloseHandle(h2);
    CloseHandle(h3);
    return 0;
}
