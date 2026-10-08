/*
 * ЛР №6, базовый вариант: первый дочерний процесс (child1.exe).
 *
 * Получает своё имя в командной строке (его назначил родитель), запускает
 * свой дочерний процесс grand1.exe (внука исходного родителя), назначая ему
 * имя Vasily и передавая его в командной строке, и выводит 12 сообщений о своём
 * шаге с паузой 1400 мс.
 *
 * Сборка:  gcc child1.c -o child1.exe -Wall
 */
#include "../common.h"

#define STEPS    12
#define DELAY_MS 1400

int main(int argc, char **argv)
{
    init_console();
    const char *myname = argc > 1 ? argv[1] : "(без имени)";
    say("Дочерний \"%s\": запущен. Имя мне назначил родитель.", myname);

    PROCESS_INFORMATION pi;
    if (start_process("grand1.exe Vasily", FALSE, &pi)) {
        say("Дочерний \"%s\": запустил своего потомка, имя Vasily, PID %lu.",
            myname, pi.dwProcessId);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);   /* хэндлы больше не нужны */
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
