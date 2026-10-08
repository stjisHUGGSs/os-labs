/*
 * ЛР №5, базовый вариант (Windows): события мыши в текстовой консоли.
 *
 *  - читает text.txt и выводит его содержимое в консоль;
 *  - включает приём событий мыши и в цикле читает их ReadConsoleInput;
 *  - по щелчку ЛЕВОЙ кнопкой на непустой позиции экрана в ПОСЛЕДНЕЙ строке окна
 *    выводит символ из этой позиции и его координаты (строка, столбец);
 *    на пробеле и пустом месте ничего не выводится;
 *  - щелчок ПРАВОЙ кнопкой завершает программу.
 *
 * Сборка:  gcc lab5.c -o lab5.exe -Wall
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define FNAME "text.txt"

static HANDLE hIn, hOut;

/* Загружает файл и выводит в консоль. Возвращает 0 при успехе. */
static int show_file(void)
{
    HANDLE f = CreateFile(FNAME, GENERIC_READ, FILE_SHARE_READ, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 1;
    char buf[4096];
    DWORD n = 0, w;
    ReadFile(f, buf, sizeof(buf), &n, NULL);
    CloseHandle(f);
    WriteFile(hOut, buf, n, &w, NULL);       /* консоль в UTF-8 (см. main) */
    return 0;
}

/* Выводит строку text в последнюю строку окна консоли цветом attr.
 * Старое содержимое этой строки предварительно стирается. */
static void status(int row, WORD defAttr, WORD attr, const wchar_t *text, int width)
{
    COORD pos = { 0, (SHORT)row };
    DWORD n;
    FillConsoleOutputCharacterW(hOut, L' ', width, pos, &n);   /* стереть строку */
    FillConsoleOutputAttribute(hOut, defAttr, width, pos, &n);
    int len = (int)wcslen(text);
    FillConsoleOutputAttribute(hOut, attr, len, pos, &n);      /* цвет для текста */
    WriteConsoleOutputCharacterW(hOut, text, len, pos, &n);    /* сам текст */
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (!GetConsoleScreenBufferInfo(hOut, &bi)) {
        fprintf(stderr, "Нужна консоль: вывод не должен быть переадресован.\n");
        return 1;
    }
    if (show_file()) {
        printf("Не удалось открыть файл " FNAME "\n");
        return 1;
    }

    /* Режим консольного ввода. Запоминаем прежний, чтобы восстановить в конце.
     * ENABLE_MOUSE_INPUT — получать события мыши. ENABLE_EXTENDED_FLAGS нужен,
     * чтобы флаг QUICK_EDIT мог быть изменён: при включённом «быстром выделении»
     * щелчок мыши используется консолью для выделения текста и до программы
     * не доходит, поэтому этот флаг снимаем. */
    DWORD oldMode;
    GetConsoleMode(hIn, &oldMode);
    DWORD mode = (oldMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS)
                 & ~ENABLE_QUICK_EDIT_MODE;
    if (!SetConsoleMode(hIn, mode)) return 1;

    int statusRow = bi.srWindow.Bottom;       /* последняя строка окна */
    int width = bi.dwSize.X;
    DWORD prevButtons = 0;                    /* состояние кнопок в прошлом событии */
    INPUT_RECORD rec;
    DWORD got;

    for (;;) {
        /* Вызов блокирует программу, пока не появится событие ввода. */
        if (!ReadConsoleInput(hIn, &rec, 1, &got) || got == 0) break;
        if (rec.EventType != MOUSE_EVENT) continue;    /* клавиши и др. не интересуют */

        MOUSE_EVENT_RECORD m = rec.Event.MouseEvent;
        DWORD pressed = m.dwButtonState & ~prevButtons;   /* кнопки, нажатые именно сейчас */
        prevButtons = m.dwButtonState;

        if (pressed & RIGHTMOST_BUTTON_PRESSED) break;    /* правая кнопка — выход */

        if (pressed & FROM_LEFT_1ST_BUTTON_PRESSED) {     /* нажатие левой кнопки */
            if (m.dwMousePosition.Y == statusRow) continue;   /* строка сообщений */

            /* Читаем с экрана один символ в позиции щелчка. */
            wchar_t ch = L' ';
            DWORD n;
            ReadConsoleOutputCharacterW(hOut, &ch, 1, m.dwMousePosition, &n);
            if (n == 0 || ch == L' ' || ch == 0) continue;   /* пусто — ничего не выводим */

            wchar_t msg[64];
            _snwprintf(msg, 64, L"Символ '%lc' (код %u), строка %d, столбец %d (с нуля)",
                       ch, (unsigned)ch, m.dwMousePosition.Y, m.dwMousePosition.X);
            status(statusRow, bi.wAttributes,
                   FOREGROUND_GREEN | FOREGROUND_INTENSITY, msg, width);
        }
    }

    SetConsoleMode(hIn, oldMode);              /* вернуть прежний режим */
    COORD end = { 0, (SHORT)statusRow };
    SetConsoleCursorPosition(hOut, end);
    printf("\n");
    return 0;
}
