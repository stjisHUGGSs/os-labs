/*
 * ЛР №6, усложнённый вариант: первый дочерний процесс (child1.exe).
 *
 * Отличие от базового: потомку передаётся не только его имя (Vasily), но и
 * СОБСТВЕННОЕ имя этого процесса — как отчество в русском языке.
 * Командная строка запуска потомка:  grand1.exe <имя потомка> <моё имя>
 *
 * Сборка:  gcc ext/child1.c -o ext/child1.exe -Wall
 */
#include "../common.h"

#define STEPS    12
#define DELAY_MS 1400

int main(int argc, char **argv)
{
    init_console();
    const char *myname = argc > 1 ? argv[1] : "(без имени)";
    say("Дочерний \"%s\": запущен. Имя мне назначил родитель.", myname);

    /* Собираем командную строку: имя программы, имя потомка, своё имя. */
    char cmd[200];
    snprintf(cmd, sizeof(cmd), "grand1.exe Vasily %s", myname);

    PROCESS_INFORMATION pi;
    if (start_process(cmd, FALSE, &pi)) {
        say("Дочерний \"%s\": запустил потомка, имя Vasily, PID %lu, передал ему своё имя.",
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
