# ЛР №2 — файлы, хэндлы и управляющие блоки файлов

| Файл | Что это |
|---|---|
| `lab2_create.c` | Linux, программа 1: создаёт `data.txt` из стандартного ввода |
| `lab2_read.c` | Linux, программа 2 (базовый вариант): `open`, `dup`, `open`, `lseek`, три чтения по 7 символов |
| `lab2_win.c` | Windows (усложнённый вариант): то же с `DuplicateHandle` |
| `input.txt` | Текст для создания файла |
| `data.txt` | Готовый файл с данными (то, что создаёт `lab2_create` из `input.txt`) |

Все команды выполняются из папки `lab02`.

## Базовый вариант (Linux)

```
gcc lab2_create.c -o lab2_create.out -Wall
gcc lab2_read.c -o lab2_read.out -Wall
./lab2_create.out <input.txt
./lab2_read.out
```

## Усложнённый вариант (Windows)

```
gcc lab2_win.c -o lab2_win.exe -Wall
.\lab2_win.exe
```

Результаты и объяснение — в `reports/lab02.md`.
