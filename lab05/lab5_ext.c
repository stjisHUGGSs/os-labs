/*
 * ЛР №5, усложнённый вариант (Windows): та же программа, но по щелчку левой
 * кнопкой вместо сведений о символе в последней строке сам символ ОТОБРАЖАЕТСЯ
 * НА СВОЁМ МЕСТЕ в другом регистре (строчная <-> прописная) и перекрашивается в
 * яркий цвет (ярко-красный). Координаты не выводятся. Правая кнопка — выход.
 *
 * Сборка:  gcc lab5_ext.c -o lab5_ext.exe -Wall
 */
#include <windows.h>
#include <stdio.h>

#define FNAME "text.txt"
#define BRIGHT_RED (FOREGROUND_RED | FOREGROUND_INTENSITY)   /* цвет перекрашенных символов */

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
    WriteFile(hOut, buf, n, &w, NULL);
    return 0;
}

/* Меняет регистр буквы. CharUpperW/CharLowerW знают не только латиницу, но и
 * кириллицу. Если символ — не буква (цифра, знак), он остаётся прежним. */
static wchar_t swap_case(wchar_t c)
{
    wchar_t up  = (wchar_t)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)c);
    wchar_t low = (wchar_t)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)c);
    if (c == low && c != up) return up;     /* была строчной -> делаем прописной */
    if (c == up && c != low) return low;    /* была прописной -> делаем строчной */
    return c;
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

    /* Включаем мышь и снимаем «быстрое выделение» (оно перехватывает щелчки). */
    DWORD oldMode;
    GetConsoleMode(hIn, &oldMode);
    DWORD mode = (oldMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS)
                 & ~ENABLE_QUICK_EDIT_MODE;
    if (!SetConsoleMode(hIn, mode)) return 1;

    DWORD prevButtons = 0;
    INPUT_RECORD rec;
    DWORD got;

    for (;;) {
        if (!ReadConsoleInput(hIn, &rec, 1, &got) || got == 0) break;
        if (rec.EventType != MOUSE_EVENT) continue;

        MOUSE_EVENT_RECORD m = rec.Event.MouseEvent;
        DWORD pressed = m.dwButtonState & ~prevButtons;   /* только что нажатые кнопки */
        prevButtons = m.dwButtonState;

        if (pressed & RIGHTMOST_BUTTON_PRESSED) break;    /* выход */

        if (pressed & FROM_LEFT_1ST_BUTTON_PRESSED) {
            wchar_t ch = L' ';
            DWORD n;
            ReadConsoleOutputCharacterW(hOut, &ch, 1, m.dwMousePosition, &n);
            if (n == 0 || ch == L' ' || ch == 0) continue;     /* пусто — пропускаем */

            wchar_t sw = swap_case(ch);
            /* На том же месте: сначала цвет, затем символ другого регистра. */
            FillConsoleOutputAttribute(hOut, BRIGHT_RED, 1, m.dwMousePosition, &n);
            WriteConsoleOutputCharacterW(hOut, &sw, 1, m.dwMousePosition, &n);
        }
    }

    SetConsoleMode(hIn, oldMode);
    COORD end = { 0, bi.srWindow.Bottom };
    SetConsoleCursorPosition(hOut, end);
    printf("\n");
    return 0;
}
