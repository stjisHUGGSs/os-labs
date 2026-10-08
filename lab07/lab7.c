/*
 * ЛР №7, базовый вариант (Linux): три нити, вывод в «свои» столбцы экрана,
 * отмена нитей (pthread_cancel) и управление отменяемостью.
 *
 * Общие данные — массив из 20 первых латинских букв. Каждая дополнительная
 * нить на своём k-м шаге выводит k-ю букву в строку k+1 своего столбца, причём
 * буква повторяется (2 * номер нити) раз, затем ждёт случайное время.
 * Нити окрашены в разные цвета, главная нить тоже выводит в свой столбец.
 *
 *   главная нить, шаг 6  — пытается отменить нить 1 (нить 1 запретила отмену);
 *   главная нить, шаг 11 — пытается отменить нить 3 (она тоже запретила);
 *   нить 3, шаг 13       — разрешает отмену в отложенном режиме;
 *   нить 3, между 16 и 17 шагами — точка отмены (pthread_testcancel), где
 *                          накопившаяся отмена наконец выполняется.
 *
 * Все служебные сообщения выводятся в фиксированные строки экрана (без прокрутки).
 * Терминал должен иметь не менее 32 строк.
 *
 * Сборка:  gcc lab7.c -o lab7.out -Wall -pthread
 */
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LETTERS 20
static const char lbuk[LETTERS + 1] = "abcdefghijklmnopqrst";   /* общие данные */

/* Цвета ANSI: ESC[...m (пособие, табл. 4.1). */
#define C_MAIN "1;37"   /* главная нить — белый   */
#define C_T1   "1;34"   /* нить 1 — синий         */
#define C_T2   "1;32"   /* нить 2 — зелёный       */
#define C_T3   "1;31"   /* нить 3 — красный       */
#define C_MSG  "1;33"   /* сообщения — жёлтый     */

/* Столбцы экрана (с 1) для вывода букв. */
#define COL_MAIN 2
#define COL_T1   20
#define COL_T2   40
#define COL_T3   60

/* Строки служебных сообщений (под областью букв, которая занимает строки 1..20). */
#define ROW_MAIN6   22
#define ROW_MAIN11  23
#define ROW_T1      24
#define ROW_T3      25
#define ROW_JOIN1   27
#define ROW_JOIN2   28
#define ROW_JOIN3   29
#define ROW_END     31

#define MAIN_DELAY_MS 200        /* пауза главной нити между шагами */
#define MIN_DELAY_MS  200        /* случайная пауза нитей: от ... */
#define MAX_DELAY_MS  600        /* ... до */

static pthread_mutex_t scr = PTHREAD_MUTEX_INITIALIZER;   /* защищает экран */
static pthread_t tid1, tid2, tid3;

/* ------------------------------------------------------------------ вывод */

/* Выводит text в позицию (row, col) цветом color.
 * Позиция, цвет и текст — несколько отдельных действий, которые нельзя
 * разрывать вмешательством другой нити, поэтому они в критическом интервале
 * (мьютекс). Внутри него отмена нити запрещена: иначе нить, отменённая в
 * середине, навсегда оставила бы мьютекс занятым и остальные зависли бы. */
static void put(int row, int col, const char *color, const char *text)
{
    int old;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    pthread_mutex_lock(&scr);
    printf("\033[%d;%dH\033[%sm%s\033[0m", row, col, color, text);
    fflush(stdout);
    pthread_mutex_unlock(&scr);
    pthread_setcancelstate(old, NULL);       /* вернуть прежнее состояние отмены */
}

/* Служебное сообщение в начало строки row (строка предварительно очищается ESC[K). */
static void msg(int row, const char *fmt, ...)
{
    char text[200], line[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    snprintf(line, sizeof(line), "\033[K%s", text);
    put(row, 1, C_MSG, line);
}

/* Пауза. usleep — точка отмены, поэтому на время сна отмену запрещаем:
 * нить отменяется только в тех местах, где мы этого сами хотим. */
static void delay_ms(int ms)
{
    int old;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
    usleep(ms * 1000);
    pthread_setcancelstate(old, NULL);
}

/* Случайная пауза MIN..MAX мс (rand_r — безопасная для нитей версия rand). */
static int random_delay(unsigned *seed)
{
    return MIN_DELAY_MS + (int)(rand_r(seed) % (MAX_DELAY_MS - MIN_DELAY_MS + 1));
}

/* Строка из letter, повторённой n раз. */
static void repeated(char *dst, char letter, int n)
{
    memset(dst, letter, n);
    dst[n] = '\0';
}

/* ------------------------------------------------------------------- нити */

/* Нить 1: синяя, повторяет букву 2 раза. Запрещает свою отмену и не разрешает
 * её никогда, поэтому приказ главной нити остаётся без последствий. */
static void *thread1(void *arg)
{
    (void)arg;
    unsigned seed = (unsigned)time(NULL) ^ 0x1111;
    char buf[16];

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    msg(ROW_T1, "Нить 1: отмена запрещена (PTHREAD_CANCEL_DISABLE).");

    for (int k = 0; k < LETTERS; k++) {
        repeated(buf, lbuk[k], 2 * 1);
        put(k + 1, COL_T1, C_T1, buf);
        delay_ms(random_delay(&seed));
    }
    return NULL;
}

/* Нить 2: зелёная, повторяет букву 4 раза, отмене не подвергается. */
static void *thread2(void *arg)
{
    (void)arg;
    unsigned seed = (unsigned)time(NULL) ^ 0x2222;
    char buf[16];

    for (int k = 0; k < LETTERS; k++) {
        repeated(buf, lbuk[k], 2 * 2);
        put(k + 1, COL_T2, C_T2, buf);
        delay_ms(random_delay(&seed));
    }
    return NULL;
}

/* Нить 3: красная, повторяет букву 6 раз. Сначала запрещает отмену. На 13-м
 * шаге разрешает её в отложенном режиме (отмена возможна только в точках
 * отмены). Единственная точка отмены — pthread_testcancel() между 16-м и 17-м
 * шагами: если к этому моменту главная нить уже потребовала отмену, нить
 * завершается здесь. */
static void *thread3(void *arg)
{
    (void)arg;
    unsigned seed = (unsigned)time(NULL) ^ 0x3333;
    char buf[16];

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    msg(ROW_T3, "Нить 3: отмена запрещена (PTHREAD_CANCEL_DISABLE).");

    for (int k = 0; k < LETTERS; k++) {
        repeated(buf, lbuk[k], 2 * 3);
        put(k + 1, COL_T3, C_T3, buf);

        if (k + 1 == 13) {                                     /* 13-й шаг */
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            msg(ROW_T3, "Нить 3, шаг 13: отмена разрешена, режим отложенный (DEFERRED).");
        }
        if (k + 1 == 16) {                                     /* между 16-м и 17-м */
            msg(ROW_T3, "Нить 3: шаг 16 выполнен, точка отмены pthread_testcancel().");
            pthread_testcancel();       /* если отмена запрошена — нить завершится здесь */
            msg(ROW_T3, "Нить 3: отмена не была запрошена, продолжаю.");
        }
        delay_ms(random_delay(&seed));
    }
    return NULL;
}

/* ------------------------------------------------------------------ main */

/* Сообщение о результате pthread_join для нити number. */
static void report(int row, int number, pthread_t tid)
{
    void *res;
    pthread_join(tid, &res);
    if (res == PTHREAD_CANCELED)
        msg(row, "Нить %d завершена по отмене (pthread_join вернул PTHREAD_CANCELED).", number);
    else
        msg(row, "Нить %d завершилась сама, отмена не выполнена.", number);
}

int main(void)
{
    printf("\033[2J");                    /* очистка экрана */
    fflush(stdout);

    pthread_create(&tid1, NULL, thread1, NULL);
    pthread_create(&tid2, NULL, thread2, NULL);
    pthread_create(&tid3, NULL, thread3, NULL);

    char buf[8];
    for (int k = 0; k < LETTERS; k++) {
        snprintf(buf, sizeof(buf), "%c++", lbuk[k]);
        put(k + 1, COL_MAIN, C_MAIN, buf);

        if (k + 1 == 6) {                                      /* 6-й шаг главной нити */
            int rc = pthread_cancel(tid1);
            msg(ROW_MAIN6, "Главная нить, шаг 6: pthread_cancel(нить 1) вернул %d (запрос принят).", rc);
        }
        if (k + 1 == 11) {                                     /* 11-й шаг */
            int rc = pthread_cancel(tid3);
            msg(ROW_MAIN11, "Главная нить, шаг 11: pthread_cancel(нить 3) вернул %d (запрос принят).", rc);
        }
        delay_ms(MAIN_DELAY_MS);
    }

    report(ROW_JOIN1, 1, tid1);
    report(ROW_JOIN2, 2, tid2);
    report(ROW_JOIN3, 3, tid3);

    printf("\033[%d;1H\033[0m\n", ROW_END);
    return 0;
}
