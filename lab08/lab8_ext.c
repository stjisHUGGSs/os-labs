/*
 * ЛР №8, усложнённый вариант (Linux, POSIX): задача «читатели-писатели» на семафорах.
 *
 * Три нити-писателя и три нити-читателя работают с общим буфером из 12 символов.
 *   писатель 1 пишет "Novosibirsk", 2 — "Semipalatink", 3 — "Ekaretinburg"
 *   (названия в точности из задания; у первого 11 букв + завершающий нуль = 12
 *   байтов, у остальных по 12 букв без нуля);
 *   каждый писатель записывает название в два приёма: первые 6 байтов (без
 *   завершающего нуля), пауза до 1 с, остальные 6 байтов; затем ждёт 2–3 с;
 *   читатели через случайное время до 300 мс читают весь буфер и выводят
 *   прочитанный текст в новую строку своего столбца (22 строки на читателя).
 *
 * Синхронизация — схема из пособия (рис. 9.1) на двух двоичных семафорах:
 *   Reader:  P(S); N=N+1; если N==1 то P(W); V(S);  чтение;  P(S); N=N-1; если N==0 то V(W); V(S)
 *   Writer:  P(W);  запись;  V(W)
 * S защищает счётчик читателей N, W закрывает буфер от писателей.
 * Оба семафора инициализируются значением 1 («свободно»). В методичке написано
 * S = 0, но при таком значении первый же P(S) читателя заблокировался бы навсегда,
 * а по определению из пособия 0 означает «занято».
 *
 * Отличие от базового варианта: вместо фиксированных 1 с (пауза писателя между
 * половинами текста) и 300 мс (интервал между чтениями) максимальные значения
 * задаются параметрами запуска в единицах 0,1 с:
 *     ./lab8_ext.out <пауза писателя> <интервал читателя>
 * например, "9 2" означает 0,9 с и 200 мс. Без параметров берутся 10 и 3.
 *
 * Для сравнения программу можно собрать БЕЗ синхронизации (-DNO_SYNC): тогда
 * читатели увидят «смесь» из частей разных названий.
 *
 * Сборка:  gcc lab8_ext.c -o lab8_ext.out -Wall -pthread        (терминал не уже 100x30)
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUF_SIZE   12           /* размер общего буфера, символов */
#define HALF       (BUF_SIZE / 2)
#define READS      22           /* число выводов каждого читателя (20–24) */
#define FIRST_ROW  4            /* строка экрана для первого события */
#define COL_WIDTH  16           /* ширина столбца каждой нити */

static int max_half_ms = 1000;  /* пауза писателя между половинами, не более, мс (по умолчанию 1 с) */
static int max_read_ms = 300;   /* интервал между чтениями читателя, не более, мс (по умолчанию 300 мс) */

#ifdef NO_SYNC
#define P(s) ((void)0)          /* без синхронизации операции семафоров ничего не делают */
#define V(s) ((void)0)
#else
#define P(s) sem_wait(s)        /* P — «проверить и захватить» */
#define V(s) sem_post(s)        /* V — «освободить» */
#endif

static char buffer[BUF_SIZE];   /* общая область данных */
static int  N = 0;              /* сколько читателей читает сейчас */
static sem_t S, W;              /* S — для счётчика N, W — для записи */

static pthread_mutex_t scr = PTHREAD_MUTEX_INITIALIZER;   /* защищает вывод на экран */
static volatile int done = 0;   /* читатели закончили — писателям пора завершаться */
static int is_tty;

static const char *names[3]  = { "Novosibirsk", "Semipalatink", "Ekaretinburg" };
static const char *colors[6] = { "1;33", "1;33", "1;33",       /* писатели — жёлтые */
                                 "1;36", "1;32", "1;35" };     /* читатели: голубой, зелёный, малиновый */

/* ------------------------------------------------------------------ вывод */

/* Выводит text в новую строку столбца column (0..5) цветом этого столбца.
 * Позиция + цвет + текст выполняются под мьютексом, чтобы нити не перемешивали
 * вывод. rows[] — сколько строк уже занято в каждом столбце. */
static void show(int column, const char *fmt, ...)
{
    static int rows[6];
    char text[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    pthread_mutex_lock(&scr);
    int row = FIRST_ROW + rows[column]++;
    if (!is_tty)
        printf("%s %d: %s\n", column < 3 ? "писатель" : "читатель", column % 3 + 1, text);
    else if (rows[column] <= READS)                     /* ниже 22 строк не выводим */
        printf("\033[%d;%dH\033[%sm%-*s\033[0m", row, column * COL_WIDTH + 1,
               colors[column], COL_WIDTH - 1, text);
    fflush(stdout);
    pthread_mutex_unlock(&scr);
}

/* Случайное число миллисекунд от 1 до max. */
static int random_ms(unsigned *seed, int max)
{
    return 1 + (int)(rand_r(seed) % (unsigned)max);
}

/* ------------------------------------------------------------------- нити */

/* Писатель с номером (int)arg = 0..2. */
static void *writer(void *arg)
{
    int id = (int)(long)arg;
    unsigned seed = (unsigned)time(NULL) * 31u + (unsigned)id;
    char rec[BUF_SIZE];                       /* 12 байтов, которые нужно записать */
    memset(rec, 0, sizeof(rec));
    memcpy(rec, names[id], strlen(names[id]) < BUF_SIZE ? strlen(names[id]) : BUF_SIZE);

    while (!done) {
        P(&W);                                /* закрыть буфер для читателей и писателей */
        memcpy(buffer, rec, HALF);            /* 1-я половина, без завершающего нуля */
        show(id, "[1/2] %.6s", rec);
        usleep(random_ms(&seed, max_half_ms) * 1000);   /* буфер сейчас «разорван» */
        memcpy(buffer + HALF, rec + HALF, HALF);        /* 2-я половина */
        show(id, "[2/2] готово");
        V(&W);

        int wait_ms = 2000 + random_ms(&seed, 1000);    /* пауза 2–3 с, шагами по 100 мс */
        for (int t = 0; t < wait_ms && !done; t += 100)
            usleep(100 * 1000);
    }
    return NULL;
}

/* Читатель с номером (int)arg = 0..2. */
static void *reader(void *arg)
{
    int id = (int)(long)arg;
    unsigned seed = (unsigned)time(NULL) * 17u + (unsigned)id;

    for (int r = 0; r < READS; r++) {
        usleep(random_ms(&seed, max_read_ms) * 1000);

        P(&S);                                /* вход: захватить счётчик читателей */
        N = N + 1;
        if (N == 1) P(&W);                    /* первый читатель закрывает буфер писателям */
        V(&S);

        char local[BUF_SIZE + 1];             /* чтение (копируем буфер к себе) */
        usleep(30 * 1000);                    /* «чтение» занимает время: читатели могут перекрываться */
        memcpy(local, buffer, BUF_SIZE);
        local[BUF_SIZE] = '\0';               /* в буфере нуля может не быть (12 букв) */

        P(&S);                                /* выход */
        N = N - 1;
        if (N == 0) V(&W);                    /* последний читатель открывает буфер писателям */
        V(&S);

        show(3 + id, "%s", local);            /* вывод — уже вне критического интервала */
    }
    return NULL;
}

/* Разбор параметра: целое положительное число (единицы 0,1 с). Возвращает мс. */
static int parse_tenths(const char *text, const char *what)
{
    char *end;
    long v = strtol(text, &end, 10);
    if (*text == '\0' || *end != '\0' || v < 1 || v > 100) {
        fprintf(stderr, "Параметр \"%s\" (%s): нужно целое от 1 до 100 (в долях 0,1 с)\n", text, what);
        exit(1);
    }
    return (int)v * 100;       /* 1 единица = 0,1 с = 100 мс */
}

int main(int argc, char **argv)
{
    if (argc > 3 || (argc > 1 && !strcmp(argv[1], "-h"))) {
        fprintf(stderr, "Использование: %s [пауза писателя] [интервал читателя]  (в долях 0,1 с)\n", argv[0]);
        return 1;
    }
    if (argc > 1) max_half_ms = parse_tenths(argv[1], "пауза писателя");
    if (argc > 2) max_read_ms = parse_tenths(argv[2], "интервал читателя");

    is_tty = isatty(STDOUT_FILENO);
    memset(buffer, '.', BUF_SIZE);            /* до первой записи в буфере точки */

    sem_init(&S, 0, 1);                       /* двоичные семафоры, начальное значение 1 */
    sem_init(&W, 0, 1);

    if (is_tty) {
        printf("\033[2J\033[1;1H");           /* очистка экрана */
        for (int c = 0; c < 6; c++)
            printf("\033[1;%dH\033[%sm%s %d\033[0m", c * COL_WIDTH + 1, colors[c],
                   c < 3 ? "Писатель" : "Читатель", c % 3 + 1);
        printf("\033[2;1H");
        for (int c = 0; c < 6 * COL_WIDTH - 1; c++) putchar('-');
        fflush(stdout);
    }

    pthread_t w[3], r[3];
    for (long i = 0; i < 3; i++) pthread_create(&w[i], NULL, writer, (void *)i);
    for (long i = 0; i < 3; i++) pthread_create(&r[i], NULL, reader, (void *)i);

    for (int i = 0; i < 3; i++) pthread_join(r[i], NULL);   /* ждём читателей */
    done = 1;                                               /* писатели завершают работу */
    for (int i = 0; i < 3; i++) pthread_join(w[i], NULL);

    if (is_tty) printf("\033[%d;1H\033[0mГотово.\n", FIRST_ROW + READS + 1);
    return 0;
}
