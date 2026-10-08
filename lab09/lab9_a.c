/*
 * ЛР №9, базовый вариант (Windows), программа 1.
 *
 *  1. Создаёт разделяемую память (CreateFileMapping + MapViewOfFile) и семафор,
 *     выводит адрес области.
 *  2. Ждёт 12 секунд (по заданию 10–15), записывает текст в разделяемую память и
 *     сообщает о его готовности, увеличив семафор (ReleaseSemaphore).
 *  3. Запрашивает 1000 байтов через VirtualAlloc и записывает в них буквы латинского
 *     алфавита через каждые 400 байтов (a — со смещением 0, b — 400, c — 800, ...),
 *     сообщая о каждой записи и её адресе. Перед записью проверяет через
 *     VirtualQuery, что страница доступна для записи, — иначе писать нельзя.
 *     С параметром "crash" запись после этого повторяется без проверки.
 *
 * Запускать ПЕРВОЙ, затем lab9_b.exe.
 * Сборка:  gcc lab9_a.c -o lab9_a.exe -Wall
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SHM_NAME  "OSLAB9_SHM"      /* имя разделяемой памяти (общее для двух программ) */
#define SEM_NAME  "OSLAB9_SEM"      /* имя семафора */
#define SHM_SIZE  4096              /* размер разделяемой области, байтов */
#define DELAY_SEC 12                /* пауза перед записью текста */
#define KEEP_SEC  15                /* сколько держим объекты, пока вторая программа читает */

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

/* Текстовое описание состояния страницы по результату VirtualQuery. */
static const char *state_name(DWORD s)
{
    return s == MEM_COMMIT ? "MEM_COMMIT (задействована)"
         : s == MEM_RESERVE ? "MEM_RESERVE (только зарезервирована)"
         : "MEM_FREE (свободна)";
}

int main(int argc, char **argv)
{
    SetConsoleOutputCP(CP_UTF8);
    say("Программа 1: создаю разделяемую память и семафор.");

    /* Первый аргумент (HANDLE)-1 вместо хэндла файла означает «память, не файл». */
    HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                    0, SHM_SIZE, SHM_NAME);
    if (!hMap) { say("CreateFileMapping: ошибка %lu", GetLastError()); return 1; }

    /* Отображаем объект в адресное пространство процесса, получаем указатель. */
    char *pm = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (!pm) { say("MapViewOfFile: ошибка %lu", GetLastError()); return 1; }
    say("Разделяемая память отображена в моё адресное пространство по адресу %p.", (void *)pm);

    /* Семафор с начальным значением 0 («данных ещё нет») и максимумом 1. */
    HANDLE hSem = CreateSemaphore(NULL, 0, 1, SEM_NAME);
    if (!hSem) { say("CreateSemaphore: ошибка %lu", GetLastError()); return 1; }

    memset(pm, 0, SHM_SIZE);
    strcpy(pm, "(данных пока нет)");
    say("Записал в память заглушку \"%s\". Жду %d с перед записью текста...", pm, DELAY_SEC);
    Sleep(DELAY_SEC * 1000);

    strcpy(pm, "Привет из первой программы! Это текст из разделяемой памяти.");
    say("Записал текст в разделяемую память: \"%s\"", pm);
    ReleaseSemaphore(hSem, 1, NULL);     /* значение семафора 0 -> 1: сообщаем о готовности */
    say("Увеличил семафор: сообщил второй программе, что текст готов.");

    /* ---- Часть 2: память через VirtualAlloc ---- */
    say("Запрашиваю 1000 байтов через VirtualAlloc.");
    char *p = VirtualAlloc(NULL, 1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!p) { say("VirtualAlloc: ошибка %lu", GetLastError()); return 1; }
    say("VirtualAlloc вернула адрес %p.", (void *)p);

    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(p, &mbi, sizeof(mbi));
    say("VirtualQuery: размер задействованного участка (RegionSize) = %lu байтов.",
        (unsigned long)mbi.RegionSize);

    int written = 0;
    for (int i = 0; i < 26; i++) {
        char *addr = p + 400 * i;                       /* смещение 0, 400, 800, ... */
        VirtualQuery(addr, &mbi, sizeof(mbi));
        BOOL writable = mbi.State == MEM_COMMIT &&
                        (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE));
        if (!writable) {
            say("Смещение %d, адрес %p: запись невозможна — страница в состоянии %s.",
                400 * i, (void *)addr, state_name(mbi.State));
            break;
        }
        *addr = (char)('a' + i);
        written++;
        say("Записал '%c' по смещению %d, адрес %p.", 'a' + i, 400 * i, (void *)addr);
    }
    say("Всего выполнено записей: %d.", written);

    if (argc > 1 && !strcmp(argv[1], "crash")) {
        say("Параметр crash: пишу по смещению 4400 без проверки...");
        *(p + 4400) = 'x';               /* здесь процесс аварийно завершится */
        say("Эта строка не выводится.");
    }

    say("Жду %d с, пока вторая программа читает память.", KEEP_SEC);
    Sleep(KEEP_SEC * 1000);

    VirtualFree(p, 0, MEM_RELEASE);
    UnmapViewOfFile(pm);
    CloseHandle(hSem);
    CloseHandle(hMap);
    say("Программа 1: конец работы.");
    return 0;
}
