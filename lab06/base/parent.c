/*
 * ЛР №6, базовый вариант: процесс-родитель.
 *
 * Запускает две дочерние программы (child1.exe, child2.exe), передавая каждой
 * текстовое имя. Затем выводит 15 сообщений о своём шаге с паузой 1 с.
 *   шаг 7  — уничтожает первую дочернюю функцией TerminateProcess
 *            (её потомок при этом продолжает работать);
 *   шаг 11 — уничтожает вторую дочернюю ВМЕСТЕ с её потомком: вторая дочерняя
 *            при запуске включена в объект Job, а TerminateJobObject
 *            уничтожает все процессы Job.
 *
 * Сборка:  gcc parent.c -o parent.exe -Wall
 * Запуск:  parent.exe   (child1.exe, child2.exe — рядом с ним)
 */
#include "../common.h"

#define STEPS    15
#define DELAY_MS 1000

int main(void)
{
    init_console();
    say("Родитель: начало работы.");

    PROCESS_INFORMATION pi1, pi2;

    /* Объект Job для второй ветви. Параметры (атрибуты защиты, имя) не нужны. */
    HANDLE job2 = CreateJobObject(NULL, NULL);
    if (!job2) { say("CreateJobObject: ошибка %lu", GetLastError()); return 1; }

    /* Первая дочерняя: обычный запуск, имя передаётся в командной строке. */
    if (!start_process("child1.exe Alexey", FALSE, &pi1)) {
        say("Не запустился child1.exe, ошибка %lu", GetLastError());
        return 1;
    }
    say("Родитель: запущен первый дочерний, имя Alexey, PID %lu.", pi1.dwProcessId);

    /* Вторая дочерняя: создаём приостановленной, включаем в Job и только затем
     * запускаем. Все процессы, которые она породит позже (потомок), автоматически
     * попадут в тот же Job. */
    if (!start_process("child2.exe Boris", TRUE, &pi2)) {
        say("Не запустился child2.exe, ошибка %lu", GetLastError());
        return 1;
    }
    if (!AssignProcessToJobObject(job2, pi2.hProcess)) {
        say("AssignProcessToJobObject: ошибка %lu", GetLastError());
        return 1;
    }
    ResumeThread(pi2.hThread);               /* теперь процесс начинает выполняться */
    say("Родитель: запущен второй дочерний, имя Boris, PID %lu (включён в Job).",
        pi2.dwProcessId);

    for (int k = 1; k <= STEPS; k++) {
        say("Родитель: шаг %d из %d.", k, STEPS);

        if (k == 7) {
            TerminateProcess(pi1.hProcess, 1);             /* только этот процесс */
            say("Родитель: приказ об уничтожении первого дочернего (PID %lu, TerminateProcess).",
                pi1.dwProcessId);
        }
        if (k == 11) {
            TerminateJobObject(job2, 1);                    /* все процессы Job */
            say("Родитель: приказ об уничтожении второго дочернего и его потомка (TerminateJobObject).");
        }
        Sleep(DELAY_MS);
    }

    /* Код завершения: у уничтоженных принудительно — тот, что мы задали (1). */
    DWORD c1 = 0, c2 = 0;
    GetExitCodeProcess(pi1.hProcess, &c1);
    GetExitCodeProcess(pi2.hProcess, &c2);
    say("Родитель: коды завершения дочерних: первый %lu, второй %lu. Конец работы.", c1, c2);

    CloseHandle(pi1.hProcess); CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess); CloseHandle(pi2.hThread);
    CloseHandle(job2);
    return 0;
}
