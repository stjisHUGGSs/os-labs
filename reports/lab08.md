<div align="center">

**МИНИСТЕРСТВО НАУКИ И ВЫСШЕГО ОБРАЗОВАНИЯ РОССИЙСКОЙ ФЕДЕРАЦИИ**<br>
Омский государственный технический университет<br>
Факультет информационных технологий и компьютерных систем

&nbsp;

**ОТЧЁТ**<br>
по лабораторной работе № 8<br>
по дисциплине «Операционные системы»<br>
**Многопоточные программы с синхронизацией**

&nbsp;

</div>

Направление 09.03.02 «Информационные системы и технологии»,<br>
профиль «Цифровые медиатехнологии, виртуальная и дополненная реальность»

Выполнил: студент группы ИСТ-251 Ковалёв М. А.<br>
Проверил: ______________________

<div align="center">

Омск, 2026

</div>

---


## 1. Цель работы

Изучить методы построения многопоточных программ с синхронизацией на примере классической
задачи «читатели-писатели».

## 2. Краткие теоретические сведения

**Абстрактные семафоры.** Семафор — защищённая от прямого доступа переменная, значение
которой опрашивают и изменяют только операциями P и V (учебное пособие, п. 9.2).
Двоичный семафор принимает значения 0 и 1: 1 означает, что ресурс *свободен*, 0 — что он
*занят*. P(S) — «проверить и захватить»: если S = 1, то S := 0, иначе нить блокируется
до освобождения. V(S) — «освободить»: S := 1 (или разблокировать ожидающую нить). Пара P и V —
«операторные скобки» критического интервала. Операции атомарны и связаны с очередями
ожидающих нитей, управляемыми ОС.

**Задача «читатели-писатели».** Несколько нитей читают общие данные, несколько пишут.
Читать одновременно могут сколько угодно читателей, но писать может только один
писатель, и при записи нет ни одного читателя. Одного семафора для этого недостаточно.
Решение использует два двоичных семафора и счётчик:

| Читатель | Писатель |
|---|---|
| P(S) | P(W) |
| N := N + 1 | запись |
| если N = 1, то P(W) | V(W) |
| V(S) | |
| чтение | |
| P(S) | |
| N := N − 1 | |
| если N = 0, то V(W) | |
| V(S) | |

W закрывает общие данные от писателей, S защищает счётчик читателей N. Первый читатель
закрывает данные для писателей, последний — открывает. В пособии и методичке начальные
значения записаны как S = 0, N = 0, W = 1. Но по определению из того же пособия 0 — «занят»:
при S = 0 первая же операция P(S) читателя заблокировала бы его навсегда, поэтому в
программе S инициализируется единицей (как и W). Это единственное расхождение с текстом.

**Средства операционных систем.** В POSIX семафор — `sem_t`: `sem_init(&s, 0, начальное)`,
`sem_wait` (аналог P), `sem_post` (аналог V). В Windows для этого служат
`CreateSemaphore`, `WaitForSingleObject` и `ReleaseSemaphore`. Мьютекс
(`pthread_mutex_lock/unlock`) отличается от двоичного семафора понятием владения: освободить его
может только нить, захватившая (п. 9.3).

## 3. Формулировка задания

**Базовый вариант.** Многопоточная программа с тремя нитями-читателями и тремя
нитями-писателями общего буфера на 12 символов. Писатели выводят латинскими буквами названия
городов: «Novosibirsk», «Semipalatink», «Ekaretinburg». Запись выполняется в два приёма: первая
половина текста (без завершающего нуля), затем пауза случайной длины до 1 с, затем вторая
половина. После записи писатель ждёт 2–3 с до следующей попытки. Читатели через случайный
интервал порядка 300 мс читают буфер (если это допускает синхронизация) и выводят прочитанный
текст на экран, каждый в свой столбец, в новую строку; на читателя выделено 20–24 строки.
Синхронизация — на семафорах. (Допускается вариант для Windows или Linux; выбран Linux.)

**Усложнённый вариант.** Вместо фиксированных 1 с и 300 мс максимальные случайные
задержки берутся из параметров запуска, выраженных целым числом десятых долей секунды
(например, для 0,9 с и 200 мс параметры «9» и «2»).

*Трактовки.* (1) Названия даны как в задании: «Novosibirsk» — 11 букв (плюс нуль —
ровно 12 байтов), «Semipalatink» и «Ekaretinburg» — по 12 букв, так что нуля в буфере
для них нет; читатель копирует 12 байтов и сам добавляет завершающий нуль в *своей*
копии. (2) «Половина» — 6 байтов из 12. (3) Читатель выполняет 22 чтения. (4) Писатель
работает, пока читатели не закончили; затем программа завершается.

## 4. Листинги

### 4.1. Базовый вариант (`lab08/lab8.c`)

```c
/*
 * ЛР №8, базовый вариант (Linux, POSIX): задача «читатели-писатели» на семафорах.
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
 * Для сравнения программу можно собрать БЕЗ синхронизации (-DNO_SYNC): тогда
 * читатели увидят «смесь» из частей разных названий.
 *
 * Сборка:  gcc lab8.c -o lab8.out -Wall -pthread        (терминал не уже 100x30)
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

static int max_half_ms = 1000;  /* пауза писателя между половинами, не более, мс */
static int max_read_ms = 300;   /* интервал между чтениями читателя, не более, мс */

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

int main(void)
{
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
```

### 4.2. Усложнённый вариант (`lab08/lab8_ext.c`)

От базового варианта отличается только разбором параметров запуска (остальной текст
совпадает; полный файл — `lab08/lab8_ext.c`):

```c
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
    if (argc > 3 || (argc > 1 && !strcmp(argv[1], "-h"))) { ... }
    if (argc > 1) max_half_ms = parse_tenths(argv[1], "пауза писателя");
    if (argc > 2) max_read_ms = parse_tenths(argv[2], "интервал читателя");
    ...
```

Сборка (предупреждений нет): `gcc lab08/lab8.c -o lab08/lab8.out -Wall -Wextra -pthread`;
аналогично для `lab8_ext.c`.

## 5. Результаты выполнения

Запуск в терминале 110×30. Слева — номер строки экрана; первые три столбца — писатели
(пишут `[1/2]` и `[2/2]`: начало записи первой и второй половины), три справа — читатели.

### 5.1. Базовый вариант, через 1,5 с после запуска

```text
01|Писатель 1      Писатель 2      Писатель 3      Читатель 1      Читатель 2      Читатель 3
02|-----------------------------------------------------------------------------------------------
04|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
05|[2/2] готово    [2/2] готово    [2/2] готово                    Ekaretinburg
06|                                                                Ekaretinburg
```

### 5.2. Базовый вариант, по окончании работы

Читатели выполнили по 22 чтения; в их столбцах — только целые названия:

```text
01|Писатель 1      Писатель 2      Писатель 3      Читатель 1      Читатель 2      Читатель 3
02|-----------------------------------------------------------------------------------------------
04|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
05|[2/2] готово    [2/2] готово    [2/2] готово    Ekaretinburg    Ekaretinburg    Ekaretinburg
06|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
07|[2/2] готово    [2/2] готово    [2/2] готово    Ekaretinburg    Ekaretinburg    Ekaretinburg
08|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
09|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
10|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
11|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
12|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
13|                                                Ekaretinburg    Ekaretinburg    Novosibirsk
14|                                                Ekaretinburg    Ekaretinburg    Semipalatink
15|                                                Novosibirsk     Ekaretinburg    Ekaretinburg
16|                                                Semipalatink    Ekaretinburg    Ekaretinburg
17|                                                Ekaretinburg    Novosibirsk     Ekaretinburg
18|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
19|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
20|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
21|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
22|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
23|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
24|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
25|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
27|Готово.
```

### 5.3. Для сравнения: та же программа без синхронизации (`-DNO_SYNC`)

Читатели видят составные «слова» из частей разных названий («Ekaretbirsk»,
«Semipabirsk», «Ekaretlatink» и т. п.), а в самом начале — ещё не записанные точки:

```text
01|Писатель 1      Писатель 2      Писатель 3      Читатель 1      Читатель 2      Читатель 3
02|-----------------------------------------------------------------------------------------------
04|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaret......    Ekaret......    Ekaret......
05|[2/2] готово    [2/2] готово    [2/2] готово    Ekaret......    Ekaretinburg    Ekaretinburg
06|                [1/2] Semipa    [1/2] Ekaret    Ekaret......    Ekaretlatink    Ekaretlatink
07|                [2/2] готово    [2/2] готово    Ekaret......    Ekaretlatink    Ekaretbirsk
08|                                                Ekaretlatink    Ekaretbirsk     Ekaretbirsk
09|                                                Ekaretlatink    Ekaretbirsk     Ekaretbirsk
10|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
11|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
12|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
13|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
14|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
15|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
16|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
17|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
18|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
19|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
20|                                                Ekaretbirsk     Ekaretbirsk     Ekaretbirsk
21|                                                Semipabirsk     Semipabirsk     Semipabirsk
22|                                                Semipabirsk     Semipabirsk     Semipabirsk
23|                                                Semipabirsk     Semipabirsk     Semipabirsk
24|                                                Ekaretbirsk     Ekaretbirsk     Semipabirsk
25|                                                Ekaretlatink    Ekaretlatink    Semipabirsk
27|Готово.
```

Для количественной проверки обе сборки запущены с выводом в файл (не терминал), и
подсчитано, какие значения прочитали читатели (66 чтений):

```text
с синхронизацией:            без синхронизации:
  29 Ekaretinburg              24 Ekaretbirsk
   6 Novosibirsk               13 Ekaretinburg
  31 Semipalatink              11 Ekaretlatink
                                5 Novosilatink
                                2 Semipabirsk
                               11 Semipalatink
```

### 5.4. Усложнённый вариант

Запуск `./lab8_ext.out 9 2` (до 0,9 с между половинами записи, до 200 мс между чтениями):

```text
01|Писатель 1      Писатель 2      Писатель 3      Читатель 1      Читатель 2      Читатель 3
02|-----------------------------------------------------------------------------------------------
04|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
05|[2/2] готово    [2/2] готово    [2/2] готово    Ekaretinburg    Ekaretinburg    Ekaretinburg
06|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
07|[2/2] готово    [2/2] готово    [2/2] готово    Ekaretinburg    Ekaretinburg    Ekaretinburg
08|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
09|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
10|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
11|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
12|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
13|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
14|                                                Ekaretinburg    Ekaretinburg    Novosibirsk
15|                                                Novosibirsk     Novosibirsk     Novosibirsk
16|                                                Novosibirsk     Novosibirsk     Novosibirsk
17|                                                Novosibirsk     Novosibirsk     Novosibirsk
18|                                                Novosibirsk     Novosibirsk     Novosibirsk
19|                                                Novosibirsk     Novosibirsk     Novosibirsk
20|                                                Semipalatink    Semipalatink    Semipalatink
21|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
22|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
23|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
24|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
25|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
27|Готово.
```

Запуск `./lab8_ext.out 2 1` (0,2 с и 100 мс) — всё происходит заметно быстрее, писатели
успевают записать меньше раз, так как читатели заканчивают раньше:

```text
01|Писатель 1      Писатель 2      Писатель 3      Читатель 1      Читатель 2      Читатель 3
02|-----------------------------------------------------------------------------------------------
04|[1/2] Novosi    [1/2] Semipa    [1/2] Ekaret    Ekaretinburg    Ekaretinburg    Ekaretinburg
05|[2/2] готово    [2/2] готово    [2/2] готово    Ekaretinburg    Ekaretinburg    Ekaretinburg
06|[1/2] Novosi                                    Ekaretinburg    Ekaretinburg    Ekaretinburg
07|[2/2] готово                                    Ekaretinburg    Ekaretinburg    Ekaretinburg
08|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
09|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
10|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
11|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
12|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
13|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
14|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
15|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
16|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
17|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
18|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
19|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
20|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
21|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
22|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
23|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
24|                                                Ekaretinburg    Ekaretinburg    Ekaretinburg
25|                                                Ekaretinburg    Ekaretinburg    Novosibirsk
27|Готово.
```

## 6. Объяснение результатов

1. **Читатели не видят «обрывков».** Писатель, захватив W, записывает первую половину,
   спит (до 1 с) и записывает вторую — на это время буфер находится в «разорванном»
   состоянии. Пока W занят, ни один читатель не может начать чтение: первый читатель
   на `P(&W)` блокируется. Поэтому в столбцах читателей (5.2) — только полные названия.
   Без семафоров (5.3) читатель ловит буфер посередине записи и получает смесь частей.
2. **Читатели работают одновременно.** В одной и той же строке разных столбцов читателей
   часто стоит одно и то же значение: они читают параллельно и не мешают друг другу. P(W)
   выполняет только первый из группы читающих, остальные просто увеличивают N, а последний
   вышедший выполняет V(W).
3. **Читатели ждут писателя.** Пока пишет писатель (до 1 с между половинами), новых строк в
   столбцах читателей нет: они заблокированы на `P(&W)` и возобновляются после `V(&W)`. Поэтому
   при параметрах по умолчанию вся программа работает около 9 с (замер), хотя без блокировок
   22 чтения по ~150 мс заняли бы около 3,3 с. При параметрах «3 1» она работает около 2,4 с.
4. **Зачем семафор S.** Счётчик N изменяют все читатели; изменения защищены
   вторым двоичным семафором S (иначе два читателя могли бы одновременно увеличить N и
   потерять одно приращение). Решение «N == 1 → P(W)» гарантирует, что W захватывается один раз
   на всю группу одновременно читающих.
5. **Повтор значений.** Один писатель после записи ждёт 2–3 с, а читатели читают каждые 0–300 мс,
   поэтому одно и то же название читается много раз подряд; смена названия видна в
   столбцах читателей после очередной записи.
6. **Параметры запуска (усложнённый вариант).** Значение параметра умножается на 100 мс и
   становится верхней границей случайной задержки; неверные параметры (не число, 0, больше
   100) отклоняются сообщением.

## 7. Ответы на контрольные вопросы

**1. Чем абстрактные семафоры отличаются от известных вам программных реализаций.**
Абстрактный семафор (п. 9.2 пособия) — идеальная переменная с операциями P и V
(двоичный: 0 и 1; считающий: неотрицательные значения; обобщённый — и отрицательные),
операции атомарны и связаны с очередями блокированных нитей. В реальных ОС его реализуют
конкретными объектами с дополнительными свойствами: в Windows это `CreateSemaphore` (внутренний
счётчик и верхняя граница, неименованный или именованный, ожидание функцией
`WaitForSingleObject` с тайм-аутом, увеличение — `ReleaseSemaphore` на заданное число), в POSIX —
`sem_t` с `sem_wait`/`sem_post`; есть и родственные объекты: мьютексы (с понятием
владения), события, критические секции. Реализации допускают ожидание с тайм-аутом, групповое
ожидание нескольких объектов, использование между процессами, а в абстрактной схеме нет
ни тайм-аута, ни максимума значения, ни владельца.

**2. Укажите в каких ситуациях предпочтительней использование мьютексов вместо
семафоров и в каких ситуациях целесообразно обратное предпочтение.**
Мьютекс удобен для *взаимного исключения* — защиты критического интервала, где одна и
та же нить входит и выходит: освободить мьютекс может только его владелец, что защищает
от ошибок (в нашей программе так защищён вывод на экран). Семафор нужен, когда
захватывает и освобождает *разные* нити — для сигнализации, подсчёта ресурсов (производитель-потребитель) и
для схемы читатели-писатели: семафор W захватывает первый читатель, а освобождает *последний*,
то есть, возможно, другая нить; мьютекс здесь неприменим.
