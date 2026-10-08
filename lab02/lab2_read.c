/*
 * ЛР №2, программа 2 (Linux), базовый вариант: три хэндла одного файла.
 *
 *   fd1 = open(...)          — первое открытие файла;
 *   fd2 = dup(fd1)           — второй номер для ТОГО ЖЕ управляющего блока;
 *   fd3 = open(...)          — повторное открытие: НОВЫЙ управляющий блок.
 *
 * Затем позиция устанавливается через fd1 на 10-й байт, выводятся числовые
 * значения хэндлов, и из файла по очереди через fd1, fd2, fd3 читается
 * по 7 символов, каждый кусок выводится на экран в своей строке.
 *
 * Сборка:  gcc lab2_read.c -o lab2_read.out -Wall
 * Запуск:  ./lab2_read.out        (файл data.txt создаёт lab2_create)
 */
#include <fcntl.h>   /* open */
#include <stdio.h>   /* perror, dprintf */
#include <unistd.h>  /* read, close, dup, lseek */

#define FNAME "data.txt"
#define PORTION 7        /* сколько символов читаем за раз */
#define START   10       /* позиция для lseek */

/* Читает PORTION байтов через хэндл fd и выводит их в отдельной строке. */
static void read_and_print(const char *name, int fd)
{
    char buf[PORTION + 1];
    ssize_t n = read(fd, buf, PORTION);   /* позиция берётся из управляющего блока */
    if (n < 0) {
        perror("read");
        return;
    }
    buf[n] = '\0';
    /* dprintf пишет сразу в дескриптор 1 без буфера stdio */
    dprintf(STDOUT_FILENO, "%s: %s\n", name, buf);
}

int main(void)
{
    /* 1-й хэндл: обычное открытие для чтения. */
    int fd1 = open(FNAME, O_RDONLY);
    if (fd1 == -1) {
        perror("open #1");
        return 1;
    }

    /* 2-й хэндл: dup даёт новый (минимальный свободный) номер, но он указывает
     * на тот же управляющий блок, а значит, и на ту же текущую позицию. */
    int fd2 = dup(fd1);
    if (fd2 == -1) {
        perror("dup");
        return 1;
    }

    /* 3-й хэндл: второе открытие того же файла создаёт отдельный управляющий
     * блок со своей собственной позицией (она начинается с 0). */
    int fd3 = open(FNAME, O_RDONLY);
    if (fd3 == -1) {
        perror("open #3");
        return 1;
    }

    /* Позиционируем чтение через ПЕРВЫЙ хэндл: 10-я позиция от начала файла. */
    if (lseek(fd1, START, SEEK_SET) == -1) {
        perror("lseek");
        return 1;
    }

    dprintf(STDOUT_FILENO, "Значения хэндлов: fd1 = %d, fd2 = %d, fd3 = %d\n",
            fd1, fd2, fd3);

    /* По очереди используем все три хэндла. */
    read_and_print("fd1", fd1);
    read_and_print("fd2", fd2);
    read_and_print("fd3", fd3);

    /* Для объяснения результатов: текущие позиции в управляющих блоках.
     * lseek(fd, 0, SEEK_CUR) позицию не меняет, а только сообщает её. */
    dprintf(STDOUT_FILENO, "Текущие позиции: fd1 = %ld, fd2 = %ld, fd3 = %ld\n",
            (long)lseek(fd1, 0, SEEK_CUR),
            (long)lseek(fd2, 0, SEEK_CUR),
            (long)lseek(fd3, 0, SEEK_CUR));

    close(fd1);
    close(fd2);
    close(fd3);
    return 0;
}
