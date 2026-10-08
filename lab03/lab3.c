/*
 * ЛР №3, базовый вариант (Windows): открытие файла с запретом совместного
 * доступа и ожидание его освобождения другим экземпляром программы.
 *
 *  1. Программа открывает text.txt для чтения, указав в CreateFile режим
 *     разделения 0 — другим процессам файл становится недоступен.
 *  2. Если файла нет — сообщение и завершение.
 *  3. Если файл занят другим экземпляром (ERROR_SHARING_VIOLATION) — сообщение
 *     об этой причине и ожидание освобождения (повторные попытки открытия).
 *  4. После открытия читает весь файл и выводит его на экран. Файл удерживается
 *     HOLD_MS мс (чтобы запрет был виден другим экземплярам), затем закрывается.
 *
 * Сообщения — цветные, по центру окна консоли. Если вывод переадресован (не
 * консоль), сообщения выводятся обычным текстом.
 *
 * Сборка:  gcc lab3.c -o lab3.exe -Wall
 * Запуск:  два-три экземпляра в разных окнах (или lab3_run.bat)
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define FNAME    "text.txt"
#define HOLD_MS  8000   /* сколько держим файл открытым после чтения */
#define RETRY_MS 300    /* период повторных попыток открытия */

/* Цвета сообщений: комбинации констант FOREGROUND_* (см. пособие, п. 4.2). */
#define C_INFO (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)   /* голубой */
#define C_OK   (FOREGROUND_GREEN | FOREGROUND_INTENSITY)                     /* зелёный */
#define C_WAIT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)    /* жёлтый */
#define C_ERR  (FOREGROUND_RED | FOREGROUND_INTENSITY)                       /* красный */
#define C_DATA (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY) /* белый */

static HANDLE hOut;         /* хэндл экрана консоли */
static BOOL   isConsole;    /* вывод идёт в консоль (а не в файл/канал)? */
static SMALL_RECT win;      /* видимая часть буфера: окно консоли */
static WORD   defAttr;      /* исходный цвет консоли */
static int    nextRow;      /* строка экрана для следующего сообщения */

/* Подготовка экрана: узнаём размер окна, очищаем его, выбираем стартовую
 * строку так, чтобы блок сообщений приходился на середину окна. */
static void init_screen(void)
{
    CONSOLE_SCREEN_BUFFER_INFO bi;
    SetConsoleOutputCP(CP_UTF8);                         /* для printf с русским текстом */
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    isConsole = GetConsoleScreenBufferInfo(hOut, &bi);   /* для файла/канала вернёт 0 */
    if (!isConsole) return;

    win = bi.srWindow;
    defAttr = bi.wAttributes;
    int width = win.Right - win.Left + 1;
    DWORD n;
    for (int y = win.Top; y <= win.Bottom; y++) {        /* очистка видимой области */
        COORD c = { win.Left, (SHORT)y };
        FillConsoleOutputCharacterW(hOut, L' ', width, c, &n);
        FillConsoleOutputAttribute(hOut, defAttr, width, c, &n);
    }
    nextRow = (win.Top + win.Bottom) / 2 - 4;
    if (nextRow < win.Top) nextRow = win.Top;
}

/* Выводит одно сообщение цветом attr с отметкой времени.
 * В консоли — по центру очередной строки; иначе — просто строкой в stdout. */
static void say(WORD attr, const char *fmt, ...)
{
    char text[512], line[560];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    SYSTEMTIME t;
    GetLocalTime(&t);
    snprintf(line, sizeof(line), "[%02d:%02d:%02d] %s", t.wHour, t.wMinute, t.wSecond, text);

    if (!isConsole) {                                    /* переадресованный вывод */
        DWORD w;
        strcat(line, "\r\n");
        WriteFile(hOut, line, (DWORD)strlen(line), &w, NULL);
        return;
    }

    wchar_t wide[560];
    int len = MultiByteToWideChar(CP_UTF8, 0, line, -1, wide, 560) - 1;  /* без '\0' */
    int width = win.Right - win.Left + 1;
    if (len > width) len = width;

    int row = nextRow < win.Bottom ? nextRow++ : win.Bottom - 1;   /* ниже окна не уходим */
    COORD pos = { (SHORT)(win.Left + (width - len) / 2), (SHORT)row };   /* по центру */
    DWORD n;
    FillConsoleOutputAttribute(hOut, attr, len, pos, &n);        /* цвет для этих позиций */
    WriteConsoleOutputCharacterW(hOut, wide, len, pos, &n);      /* сам текст */
}

/* Выводит содержимое прочитанного файла построчно. */
static void show_data(char *data)
{
    for (char *p = strtok(data, "\n"); p; p = strtok(NULL, "\n")) {
        size_t l = strlen(p);
        if (l && p[l - 1] == '\r') p[l - 1] = '\0';
        if (*p) say(C_DATA, "%s", p);
    }
}

int main(void)
{
    init_screen();
    say(C_INFO, "Открываю файл \"" FNAME "\" с запретом совместного доступа...");

    HANDLE h;
    BOOL waited = FALSE;
    for (;;) {
        /* Третий аргумент — режим разделения. 0 = не разрешать другим ни чтение,
         * ни запись, пока файл открыт нами. */
        h = CreateFile(FNAME, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) break;

        DWORD err = GetLastError();               /* причина неудачи */
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            say(C_ERR, "Файл \"" FNAME "\" не найден (код %lu). Работа прекращена.", err);
            return 1;
        }
        if (err == ERROR_SHARING_VIOLATION) {     /* код 32: файл занят другим процессом */
            if (!waited)
                say(C_WAIT, "Файл занят другим экземпляром (код %lu). Жду освобождения...", err);
            waited = TRUE;
            Sleep(RETRY_MS);                       /* и пробуем снова */
            continue;
        }
        say(C_ERR, "Не удалось открыть файл, код ошибки %lu.", err);
        return 1;
    }

    say(C_OK, waited ? "Файл освободился, открыл его. Доступ другим запрещён."
                     : "Файл свободен, открыл его. Доступ другим запрещён.");

    char data[4096];
    DWORD got = 0;
    if (ReadFile(h, data, sizeof(data) - 1, &got, NULL)) {   /* весь небольшой файл за раз */
        data[got] = '\0';
        show_data(data);
    }

    say(C_INFO, "Держу файл %d с (чтобы запрет был заметен)...", HOLD_MS / 1000);
    Sleep(HOLD_MS);

    CloseHandle(h);       /* закрытие снимает запрет совместного доступа */
    say(C_OK, "Файл закрыт, запрет снят. Конец работы.");

    /* Если окно создано только для нашей программы (запуск из Проводника),
     * оно закроется сразу после завершения — ждём Enter, чтобы можно было
     * прочитать сообщения. При запуске из командной оболочки не ждём. */
    DWORD pids[2];
    if (isConsole && GetConsoleProcessList(pids, 2) == 1) {
        COORD end = { win.Left, win.Bottom };
        SetConsoleCursorPosition(hOut, end);
        printf("Нажмите Enter...");
        fflush(stdout);
        getchar();
    }
    return 0;
}
