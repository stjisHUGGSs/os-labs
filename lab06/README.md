# ЛР №6 — несколько процессов в Windows, объекты Job

Пять программ: родитель, два дочерних процесса, два «внука» (потомки дочерних).

| Файл | Что это |
|---|---|
| `common.h` | общие вспомогательные функции (вывод строки, запуск процесса) |
| `base/parent.c` | родитель: запускает детей, на 7-м шаге убивает первого (`TerminateProcess`), на 11-м второго с потомком (`TerminateJobObject`) |
| `base/child1.c`, `base/child2.c` | дочерние: запускают своего потомка, имя которого передают в командной строке |
| `base/grand1.c`, `base/grand2.c` | потомки (внуки родителя) |
| `ext/child1.c`, `ext/child2.c`, `ext/grand1.c`, `ext/grand2.c` | усложнённый вариант: потомку передаётся ещё и имя его родителя |

Родитель (`parent.c`) в обоих вариантах один и тот же.

## Базовый вариант

Команды выполняются из папки `lab06/base`:

```
gcc parent.c -o parent.exe -Wall
gcc child1.c -o child1.exe -Wall
gcc child2.c -o child2.exe -Wall
gcc grand1.c -o grand1.exe -Wall
gcc grand2.c -o grand2.exe -Wall
.\parent.exe
```

## Усложнённый вариант

Из папки `lab06/ext` (родитель собирается из `base/parent.c`):

```
gcc ../base/parent.c -o parent.exe -Wall
gcc child1.c -o child1.exe -Wall
gcc child2.c -o child2.exe -Wall
gcc grand1.c -o grand1.exe -Wall
gcc grand2.c -o grand2.exe -Wall
.\parent.exe
```

Все процессы пишут в одно окно консоли строки вида `[время] PID | сообщение`.
Результаты, объяснение и ответы на контрольные вопросы — в `reports/lab06.md`.
