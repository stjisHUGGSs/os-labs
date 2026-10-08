/*
 * ЛР №2, программа 1 (Linux): создаёт текстовый файл data.txt из данных,
 * введённых со стандартного ввода.
 *
 * Сборка:  gcc lab2_create.c -o lab2_create.out -Wall
 * Запуск:  ./lab2_create.out <input.txt      (или набрать строку с клавиатуры)
 */
#include <fcntl.h>   /* open, O_* */
#include <stdio.h>   /* perror */
#include <unistd.h>  /* read, write, close */

#define FNAME "data.txt"

int main(void)
{
    char buf[256];

    /* Открываем файл для записи: O_CREAT — создать, если нет;
     * O_TRUNC — обнулить, если уже был. 0600 — права rw для владельца.
     * open возвращает файловый дескриптор (хэндл) или -1 при ошибке. */
    int fd = open(FNAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* Читаем текст из стандартного ввода (дескриптор 0).
     * С клавиатуры вернётся одна строка, из файла — до sizeof(buf) байт. */
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    /* Пишем ровно столько байтов, сколько прочитали, в файл. */
    if (write(fd, buf, (size_t)n) != n) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);   /* закрытие файла: ОС освобождает его управляющий блок */
    return 0;
}
