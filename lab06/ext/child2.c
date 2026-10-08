/*
 * ЛР №6, усложнённый вариант: второй дочерний процесс (child2.exe).
 *
 * Отличие от базового: потомку передаётся не только его имя (Grigory), но и
 * СОБСТВЕННОЕ имя этого процесса — как отчество в русском языке.
 * Командная строка запуска потомка:  grand2.exe <имя потомка> <моё имя>
 *
 * Сборка:  gcc ext/child2.c -o ext/child2.exe -Wall
 */
#include "../common.h"

#define STEPS    12
#define DELAY_MS 1700

int main(int argc, char **argv)
{
    init_console();
    const char *myname = argc > 1 ? argv[1] : "(без имени)";
    say("Дочерний \"%s\": запущен. Имя мне назначил родитель.", myname);

    /* Собираем командную строку: имя программы, имя потомка, своё имя. */
    char cmd[200];
    snprintf(cmd, sizeof(cmd), "grand2.exe Grigory %s", myname);

    PROCESS_INFORMATION pi;
    if (start_process(cmd, FALSE, &pi)) {
        say("Дочерний \"%s\": запустил потомка, имя Grigory, PID %lu, передал ему своё имя.",
            myname, pi.dwProcessId);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    } else {
        say("Дочерний \"%s\": потомок не запустился, ошибка %lu.", myname, GetLastError());
    }

    for (int k = 1; k <= STEPS; k++) {
        say("Дочерний \"%s\": шаг %d из %d.", myname, k, STEPS);
        Sleep(DELAY_MS);
    }
    say("Дочерний \"%s\": конец работы.", myname);
    return 0;
}
