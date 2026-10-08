/*
 * ЛР №1, базовый вариант (Windows).
 *
 * Программа получает хэндлы стандартного ввода и вывода, выводит их значения,
 * делает приглашение, вводит текст и выводит его с предуведомлением.
 * Используются только небуферизованные системные функции ReadFile/WriteFile.
 *
 * Сборка:  gcc lab01/lab1.c -o lab01/lab1.exe -Wall
 */
#include <windows.h>
#include <stdio.h>   /* sprintf — только для форматирования строки в памяти */
#include <string.h>

int main(void)
{
    /* Кодовая страница UTF-8 для консоли: чтобы русский текст исходника
     * (он хранится в UTF-8) отображался и вводился правильно. */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* В Windows номера стандартных потоков заранее неизвестны — их нужно
     * запросить у ОС. Это и есть хэндлы стандартного ввода и вывода. */
    HANDLE hStdIn  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Результат обязательно проверяем: при неудаче вернётся INVALID_HANDLE_VALUE. */
    if (hStdIn == INVALID_HANDLE_VALUE || hStdOut == INVALID_HANDLE_VALUE) {
        return 1;
    }

    char  msg[256];
    DWORD written = 0, readBytes = 0;   /* сколько байтов реально записано/прочитано */
    int   len;

    /* Числовые значения хэндлов с комментариями. Сначала формируем строку
     * в памяти (sprintf), затем выводим её системной функцией WriteFile. */
    len = sprintf(msg,
        "Handle stdin  = %p\r\nHandle stdout = %p\r\n\r\n",
        hStdIn, hStdOut);
    WriteFile(hStdOut, msg, (DWORD)len, &written, NULL);

    /* Приглашение на ввод. Выводится безусловно в stdout — поэтому при
     * переадресации вывода в файл оно тоже окажется в файле. */
    const char *prompt = "Введите произвольный текст: ";
    WriteFile(hStdOut, prompt, (DWORD)strlen(prompt), &written, NULL);

    /* Ввод текста. ReadFile возвращает управление, когда в консоли нажат Enter
     * (или когда прочитан файл, если ввод переадресован); в readBytes — число
     * реально прочитанных байтов. */
    char buf[256];
    ReadFile(hStdIn, buf, sizeof(buf) - 1, &readBytes, NULL);
    buf[readBytes] = '\0';

    /* Предуведомление — отдельным вызовом системной функции вывода. */
    const char *notice = "\r\nРанее введённый в программу текст:\r\n";
    WriteFile(hStdOut, notice, (DWORD)strlen(notice), &written, NULL);

    /* Сам введённый текст: выводим ровно столько байтов, сколько прочитали. */
    WriteFile(hStdOut, buf, readBytes, &written, NULL);

    return 0;
}
