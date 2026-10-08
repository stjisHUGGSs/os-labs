/*
 * ЛР №9, усложнённый вариант (Linux), программа 2: открывает разделяемую память и
 * семафор, созданные программой 1.
 *
 *  1. shmget без IPC_CREAT + shmat — подключается к существующей области, выводит
 *     адрес, по которому она отображена в ЭТОМ процессе.
 *  2. Сразу читает текст и выводит его (с примечанием).
 *  3. Ждёт семафор (semop с операцией -1 блокирует, пока значение равно 0).
 *  4. После сигнала снова читает память и выводит содержимое.
 *
 * Запускать ПОСЛЕ lab9_a.out.
 * Сборка:  gcc lab9_b.c -o lab9_b.out -Wall
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define KEY_SHM  0x4C390001
#define KEY_SEM  0x4C390002
#define SHM_SIZE 4096

static void say(const char *fmt, ...)
{
    char text[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    printf("[%02d:%02d:%02d] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, text);
    fflush(stdout);
}

int main(void)
{
    say("Программа 2: открываю разделяемую память и семафор.");

    /* Область уже существует — флаг IPC_CREAT не нужен. */
    int shmid = shmget(KEY_SHM, SHM_SIZE, 0600);
    if (shmid == -1) {
        perror("shmget (сначала запустите lab9_a.out)");
        return 1;
    }
    /* SHM_RDONLY — только чтение: эта программа память не меняет. */
    const char *pb = shmat(shmid, NULL, SHM_RDONLY);
    if (pb == (const char *)-1) { perror("shmat"); return 1; }
    say("Разделяемая память (shmid %d) отображена в моё адресное пространство по адресу %p.",
        shmid, (const void *)pb);

    int semid = semget(KEY_SEM, 1, 0600);
    if (semid == -1) { perror("semget"); return 1; }

    say("Читаю разделяемую память до сигнала семафора: \"%s\"", pb);

    say("Жду сигнала семафора...");
    struct sembuf down = { .sem_num = 0, .sem_op = -1, .sem_flg = 0 };   /* P: ждать и захватить */
    semop(semid, &down, 1);                                             /* блокируется, пока 0 */
    say("Семафор сработал.");

    say("Читаю разделяемую память после сигнала: \"%s\"", pb);

    shmdt(pb);
    say("Программа 2: конец работы.");
    return 0;
}
