/*
 * ЛР №9, усложнённый вариант (Linux), программа 1: создаёт разделяемую память
 * и семафор (System V IPC), через 12 секунд записывает в память текст и
 * сообщает о готовности, увеличив семафор.
 *
 *   shmget + shmat  — получить область разделяемой памяти и её адрес;
 *   semget + semctl — создать семафор и задать ему начальное значение 0;
 *   semop(+1)       — «V»: сообщить второй программе, что текст готов.
 *
 * Запускать ПЕРВОЙ, затем lab9_b.out (из той же папки).
 * Сборка:  gcc lab9_a.c -o lab9_a.out -Wall
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define KEY_SHM   0x4C390001     /* числовые «имена» объектов, общие для двух программ */
#define KEY_SEM   0x4C390002
#define SHM_SIZE  4096
#define DELAY_SEC 12             /* пауза перед записью текста */
#define KEEP_SEC  15             /* сколько держим объекты, пока вторая программа читает */

/* Для semctl(SETVAL) в Linux нужно самому описать это объединение. */
union semun { int val; };

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
    say("Программа 1: создаю разделяемую память и семафор.");

    /* Область разделяемой памяти: IPC_CREAT — создать, 0600 — права. */
    int shmid = shmget(KEY_SHM, SHM_SIZE, IPC_CREAT | 0600);
    if (shmid == -1) { perror("shmget"); return 1; }

    /* Подключаем область к адресному пространству процесса (NULL — адрес выбирает ОС). */
    char *pm = shmat(shmid, NULL, 0);
    if (pm == (char *)-1) { perror("shmat"); return 1; }
    say("Разделяемая память (shmid %d) отображена в моё адресное пространство по адресу %p.",
        shmid, (void *)pm);

    /* Набор из одного семафора, начальное значение 0 («данных ещё нет»). */
    int semid = semget(KEY_SEM, 1, IPC_CREAT | 0600);
    if (semid == -1) { perror("semget"); return 1; }
    union semun arg = { .val = 0 };
    semctl(semid, 0, SETVAL, arg);

    memset(pm, 0, SHM_SIZE);
    strcpy(pm, "(данных пока нет)");
    say("Записал в память заглушку \"%s\". Жду %d с перед записью текста...", pm, DELAY_SEC);
    sleep(DELAY_SEC);

    strcpy(pm, "Привет из первой программы! Это текст из разделяемой памяти.");
    say("Записал текст в разделяемую память: \"%s\"", pm);

    struct sembuf up = { .sem_num = 0, .sem_op = +1, .sem_flg = 0 };   /* V: значение 0 -> 1 */
    semop(semid, &up, 1);
    say("Увеличил семафор: сообщил второй программе, что текст готов.");

    say("Жду %d с, пока вторая программа читает память.", KEEP_SEC);
    sleep(KEEP_SEC);

    shmdt(pm);                          /* отключить память от процесса */
    shmctl(shmid, IPC_RMID, NULL);      /* удалить область (сама она не исчезает!) */
    semctl(semid, 0, IPC_RMID);         /* удалить семафор */
    say("Программа 1: объекты удалены, конец работы.");
    return 0;
}
