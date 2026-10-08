/*
 * ЛР №6, усложнённый вариант: потомок (второй внук, grand2.exe).
 *
 * Получает два аргумента: своё имя и имя своего родителя. В сообщениях
 * называет оба — как «имя и отчество».
 *
 * Сборка:  gcc ext/grand2.c -o ext/grand2.exe -Wall
 */
#include "../common.h"

#define STEPS    12
#define DELAY_MS 1600

int main(int argc, char **argv)
{
    init_console();
    const char *myname = argc > 1 ? argv[1] : "(без имени)";
    const char *parent = argc > 2 ? argv[2] : "(неизвестен)";
    say("Внук \"%s\" (родитель \"%s\"): запущен. Своё имя и имя родителя получил в командной строке.",
        myname, parent);

    for (int k = 1; k <= STEPS; k++) {
        say("Внук \"%s\" (родитель \"%s\"): шаг %d из %d.", myname, parent, k, STEPS);
        Sleep(DELAY_MS);
    }
    say("Внук \"%s\" (родитель \"%s\"): конец работы.", myname, parent);
    return 0;
}
