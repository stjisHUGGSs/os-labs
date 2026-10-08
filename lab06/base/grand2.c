/*
 * ЛР №6, базовый вариант: потомок (второй внук, grand2.exe).
 * Получает своё имя в командной строке и выводит 12 сообщений о своём шаге
 * с паузой 1600 мс.
 *
 * Сборка:  gcc grand2.c -o grand2.exe -Wall
 */
#include "../common.h"

#define STEPS    12
#define DELAY_MS 1600

int main(int argc, char **argv)
{
    init_console();
    const char *myname = argc > 1 ? argv[1] : "(без имени)";
    say("Внук \"%s\": запущен. Имя мне назначил мой родитель.", myname);

    for (int k = 1; k <= STEPS; k++) {
        say("Внук \"%s\": шаг %d из %d.", myname, k, STEPS);
        Sleep(DELAY_MS);
    }
    say("Внук \"%s\": конец работы.", myname);
    return 0;
}
