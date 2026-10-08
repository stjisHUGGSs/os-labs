/*
 * ЛР №4, базовый вариант (Linux): блокировка файла по записи через fcntl.
 *
 *  1. Открывает text.txt (нет файла — сообщение и выход).
 *  2. Пытается сразу поставить блокировку по записи на весь файл (F_SETLK).
 *  3. Если файл заблокирован другим процессом — сообщает об этом (с PID
 *     владельца, его даёт F_GETLK) и ставит блокировку с ожиданием (F_SETLKW).
 *  4. Читает весь файл, выводит на экран, «засыпает» на SLEEP_SEC секунд,
 *     снимает блокировку, закрывает файл.
 *
 * Сообщения цветные (управляющие последовательности ANSI) и по центру экрана.
 * Если вывод переадресован (не терминал), сообщения идут обычными строками.
 *
 * Сборка:  gcc lab4.c -o lab4.out -Wall
 * Запуск:  по экземпляру в каждой консоли, либо ./lab4_run.sh
 */
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>   /* ioctl, TIOCGWINSZ — размер терминала */
#include <time.h>
#include <unistd.h>

#define FNAME     "text.txt"
#define SLEEP_SEC 7          /* по заданию 7 или 8 секунд */

/* Цвета: параметры управляющей последовательности ESC[...m (пособие, табл. 4.1) */
#define C_INFO "1;36"   /* ярко-бирюзовый */
#define C_OK   "1;32"   /* ярко-зелёный */
#define C_WAIT "1;33"   /* ярко-жёлтый */
#define C_ERR  "1;31"   /* ярко-красный */
#define C_DATA "1;37"   /* ярко-белый */

static int rows = 24, cols = 80;   /* размер экрана (по умолчанию, если не терминал) */
static int is_tty;
static int next_row;               /* строка для следующего сообщения */

/* Число символов (не байтов) в UTF-8 строке — для расчёта центрирования. */
static int utf8_len(const char *s)
{
    int n = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80) n++;   /* пропускаем байты-продолжения */
    return n;
}

/* Подготовка экрана: узнаём размер, очищаем (ESC[2J), выбираем стартовую строку
 * так, чтобы блок сообщений пришёлся на середину. */
static void init_screen(void)
{
    struct winsize ws;
    is_tty = isatty(STDOUT_FILENO);
    if (is_tty && ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        rows = ws.ws_row;
        cols = ws.ws_col;
        dprintf(STDOUT_FILENO, "\033[2J");
    }
    next_row = rows / 2 - 4;
    if (next_row < 1) next_row = 1;
}

/* Одно цветное сообщение по центру очередной строки (с отметкой времени). */
static void say(const char *color, const char *fmt, ...)
{
    char text[512], line[560];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    snprintf(line, sizeof(line), "[%02d:%02d:%02d] %s", tm.tm_hour, tm.tm_min, tm.tm_sec, text);

    if (!is_tty) {                       /* вывод в файл/канал — обычная строка */
        dprintf(STDOUT_FILENO, "%s\n", line);
        return;
    }
    int len = utf8_len(line);
    int col = (cols - len) / 2 + 1;      /* ANSI-координаты начинаются с 1 */
    if (col < 1) col = 1;
    int row = next_row < rows - 1 ? next_row++ : rows - 1;
    /* ESC[строка;столбецH — позиция; ESC[цвет m — цвет; ESC[0m — сброс цвета.
     * Всё одним вызовом, чтобы позиция и цвет не «разорвались» с текстом. */
    dprintf(STDOUT_FILENO, "\033[%d;%dH\033[%sm%s\033[0m", row, col, color, line);
}

int main(void)
{
    init_screen();
    say(C_INFO, "Открываю файл \"" FNAME "\"...");

    /* Блокировка по записи (F_WRLCK) возможна только для файла, открытого на запись,
     * поэтому режим O_RDWR, хотя программа только читает. */
    int fd = open(FNAME, O_RDWR);
    if (fd == -1) {
        say(C_ERR, "Не удалось открыть файл: %s. Работа прекращена.", strerror(errno));
        dprintf(STDOUT_FILENO, "\033[%d;1H\n", rows);
        return 1;
    }

    /* Описание блокируемого участка: весь файл (l_start = 0, l_len = 0 — до конца). */
    struct flock lk = { .l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0 };

    /* Попытка без ожидания. Успех — 0, неудача — -1 (errno = EACCES или EAGAIN). */
    if (fcntl(fd, F_SETLK, &lk) == 0) {
        say(C_OK, "Блокировка по записи установлена сразу.");
    } else {
        /* Выясняем, кто держит блокировку: F_GETLK заполняет lk описанием
         * первой мешающей блокировки, в том числе l_pid — PID владельца. */
        struct flock who = lk;
        if (fcntl(fd, F_GETLK, &who) == 0 && who.l_type != F_UNLCK)
            say(C_WAIT, "Файл заблокирован процессом %d. Жду освобождения...", (int)who.l_pid);
        else
            say(C_WAIT, "Файл заблокирован другим процессом. Жду освобождения...");

        /* Блокировка с ожиданием: процесс «засыпает» внутри fcntl до освобождения. */
        lk.l_type = F_WRLCK;
        if (fcntl(fd, F_SETLKW, &lk) == -1) {
            say(C_ERR, "Ошибка fcntl: %s", strerror(errno));
            return 1;
        }
        say(C_OK, "Дождался. Блокировка по записи установлена.");
    }

    /* Чтение всего файла (маленького) и вывод построчно. */
    char data[4096];
    ssize_t n = read(fd, data, sizeof(data) - 1);
    if (n > 0) {
        data[n] = '\0';
        for (char *p = strtok(data, "\n"); p; p = strtok(NULL, "\n"))
            say(C_DATA, "%s", p);
    }

    say(C_INFO, "Сплю %d с, не снимая блокировку...", SLEEP_SEC);
    sleep(SLEEP_SEC);

    /* Снятие блокировки: тот же участок, тип F_UNLCK. Блокировка и так исчезла бы
     * при закрытии файла или завершении процесса. */
    lk.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lk);
    close(fd);
    say(C_OK, "Блокировка снята, файл закрыт. Конец работы.");

    if (is_tty) dprintf(STDOUT_FILENO, "\033[%d;1H\n", rows);   /* курсор под блок */
    return 0;
}
