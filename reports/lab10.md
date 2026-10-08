<div align="center">

**МИНИСТЕРСТВО НАУКИ И ВЫСШЕГО ОБРАЗОВАНИЯ РОССИЙСКОЙ ФЕДЕРАЦИИ**<br>
Омский государственный технический университет<br>
Факультет информационных технологий и компьютерных систем

&nbsp;

**ОТЧЁТ**<br>
по лабораторной работе № 10<br>
по дисциплине «Операционные системы»<br>
**Командные файлы в Windows и Linux**

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

Сформировать умение использовать командные файлы при взаимодействии с операционной системой.

## 2. Краткие теоретические сведения

**Командный файл** (сценарий, пакетный файл) — текстовый файл с командами, которые выполняет
интерпретатор команд: в Windows — `cmd.exe`, в Linux чаще всего `bash` (учебное пособие,
п. 12.6; методические указания, ЛР №10). В Windows такой файл должен иметь расширение
`.bat` или `.cmd`, запускается по имени. В Linux ограничений на имя нет; файл можно
запустить командой `bash имя`, а чтобы запускать по имени — выдать право на выполнение
(`chmod +x`). Первая строка вида `#!/bin/bash` указывает, какой интерпретатор применять.
Комментарии: в Windows — строки, начинающиеся с `REM`, в Linux — после `#`. Вывод текста:
`echo`; в Windows `@echo off` отключает вывод самих команд.

**Переменные.** Windows: `set имя=значение` (пробелов вокруг `=` быть не должно), значение —
`%имя%`. Linux: `имя=значение` (без пробелов), значение — `$имя` или `${имя}`. Ввод строки от
пользователя: Windows — `set /p имя=приглашение`, Linux — `read имя` (с `-p` — с
приглашением). Неопределённая переменная даёт пустую строку, сообщения об ошибке нет.

**Циклы.** Windows: `for %%k in (набор) do команда` (имя переменной — одна буква);
несколько команд в теле записывают в скобках по одной в строке или соединяют знаком `&`.
Linux: `for k in набор; do …; done`.

**Отложенное раскрытие переменных в Windows.** Значение `%имя%` подставляется один раз до
выполнения всего блока (`for`, `if`), поэтому изменённая внутри блока переменная «не
видна». Выход — директива `setlocal EnableDelayedExpansion` и запись `!имя!`.

**Операции с файлами.** Windows: `copy`, `move`, `mkdir`; Linux: `cp`, `mv`, `mkdir`. Изменение
текста переменной: в Linux — `${имя%суффикс}`, `${имя/%фрагмент/замена}`, в Windows —
`%имя:текст1=текст2%` и модификаторы переменной цикла (`%%~nf` — имя файла без
расширения).

## 3. Формулировка задания

**Базовый вариант.** Командные файлы для Windows и Linux, которые запрашивают имя каталога,
создают подкаталог с этим именем в текущем каталоге и копируют в него все файлы с заданным для
студента расширением (например, `.c` или `.txt`) из текущего каталога.

**Усложнённый вариант.** Командные файлы для Windows и Linux, которые запрашивают имя каталога,
создают подкаталог, а все файлы с заданным расширением переносят во вновь создаваемые
подкаталоги этого каталога. Имя каждого подкаталога совпадает с именем переносимого файла
без расширения: файл `proga.txt` переносится в подкаталог `proga`.

*Трактовка.* Расширение, «заданное для конкретного студента», в методических указаниях не
указано, поэтому в скриптах оно вынесено в переменную `EXT` (по умолчанию `c`) — её достаточно
изменить. Скрипты работают с файлами текущего каталога.

## 4. Листинги

### 4.1. Базовый вариант, Windows (`lab10/lab10_base.bat`)

```bat
@echo off
rem Lab 10, basic variant (Windows): ask for a directory name, create it in the
rem current directory and copy into it all files with the given extension.
rem (Text is in English on purpose: cmd reads .bat files in the OEM code page,
rem and non-ASCII characters break parsing.)
rem
rem Run from the directory that holds the files:  ..\lab10_base.bat
setlocal EnableDelayedExpansion

rem EXT is the extension of the student's files. The task says: for example .c or .txt
set EXT=c

rem set /p prints a prompt and reads one line into the variable DIR.
set /p DIR=Enter directory name:

rem An empty name is not allowed.
if "%DIR%"=="" (
    echo Directory name is empty.
    exit /b 1
)

rem Create the directory unless it already exists.
if not exist "%DIR%" mkdir "%DIR%"

rem Loop over every file of the current directory matching *.c.
rem %%f is the loop variable. Several commands in the body are written in
rem parentheses, one per line, instead of joining them with the & character.
rem The counter changes inside the loop, so it is read as !COUNT! (delayed
rem expansion): %COUNT% would be substituted once, before the loop starts.
set COUNT=0
for %%f in (*.%EXT%) do (
    copy "%%f" "%DIR%" >nul
    echo Copied file: %%f
    set /a COUNT+=1
)

echo Done. Copied files .%EXT%: !COUNT!, directory: %DIR%
endlocal
```

### 4.2. Базовый вариант, Linux (`lab10/lab10_base.sh`)

```bash
#!/bin/bash
# ЛР №10, базовый вариант (Linux): спросить имя каталога, создать его в текущем
# каталоге и скопировать в него все файлы с заданным расширением.
#
# Запуск из каталога с файлами:  bash ../lab10_base.sh   (или ./lab10_base.sh)

EXT=c        # расширение файлов студента (в задании: например .c или .txt)

# read -p выводит приглашение и читает введённую строку в переменную DIR.
read -p "Введите имя каталога: " DIR

# Пустое имя каталога недопустимо.
if [ -z "$DIR" ]; then
    echo "Имя каталога не задано."
    exit 1
fi

# mkdir -p не считает ошибкой уже существующий каталог.
mkdir -p "$DIR" || exit 1

COUNT=0
# Цикл по всем файлам текущего каталога вида *.c. Кавычки вокруг "$f" нужны,
# чтобы имена с пробелами не разваливались на части.
for f in *.$EXT
do
    # Если подходящих файлов нет, шаблон остаётся строкой "*.c" — такой файл пропускаем.
    [ -f "$f" ] || continue
    cp "$f" "$DIR/"
    echo "Скопирован файл: $f"
    COUNT=$((COUNT + 1))
done

echo "Готово. Скопировано файлов .$EXT: $COUNT, каталог: $DIR"
```

### 4.3. Усложнённый вариант, Windows (`lab10/lab10_ext.bat`)

```bat
@echo off
rem Lab 10, extended variant (Windows): ask for a directory name, create it in
rem the current directory and MOVE all files with the given extension into newly
rem created subdirectories. The subdirectory name equals the file name without the
rem extension:  proga.c  ->  <directory>\proga\proga.c
rem (Text is in English on purpose: cmd reads .bat files in the OEM code page.)
rem
rem Run from the directory that holds the files:  ..\lab10_ext.bat  (files are moved!)
setlocal EnableDelayedExpansion

set EXT=c

set /p DIR=Enter directory name:

if "%DIR%"=="" (
    echo Directory name is empty.
    exit /b 1
)

if not exist "%DIR%" mkdir "%DIR%"

rem %%~nf is the file name without the extension. The same can be done with the
rem substitution form !NAME:.%EXT%=! described in the manual. NAME and COUNT
rem change inside the loop, so they are read with exclamation marks (delayed
rem expansion).
set COUNT=0
for %%f in (*.%EXT%) do (
    set NAME=%%~nf
    if not exist "%DIR%\!NAME!" mkdir "%DIR%\!NAME!"
    move "%%f" "%DIR%\!NAME!\" >nul
    echo Moved file: %%f to %DIR%\!NAME!\
    set /a COUNT+=1
)

echo Done. Moved files .%EXT%: !COUNT!, directory: %DIR%
endlocal
```

### 4.4. Усложнённый вариант, Linux (`lab10/lab10_ext.sh`)

```bash
#!/bin/bash
# ЛР №10, усложнённый вариант (Linux): спросить имя каталога, создать его в текущем
# каталоге, а все файлы с заданным расширением ПЕРЕНЕСТИ во вновь создаваемые
# подкаталоги. Имя подкаталога совпадает с именем файла без расширения:
# proga.c  ->  <каталог>/proga/proga.c
#
# Запуск из каталога с файлами:  bash ../lab10_ext.sh   (файлы будут перемещены!)

EXT=c        # расширение файлов студента

read -p "Введите имя каталога: " DIR

if [ -z "$DIR" ]; then
    echo "Имя каталога не задано."
    exit 1
fi

mkdir -p "$DIR" || exit 1

COUNT=0
for f in *.$EXT
do
    [ -f "$f" ] || continue

    # Отбрасываем расширение. Форма ${f%.c} удаляет самый короткий суффикс ".c"
    # с конца значения (в методичке аналогично: ${имя/%фрагмент/замена}).
    NAME=${f%.$EXT}

    mkdir -p "$DIR/$NAME"          # подкаталог с именем файла без расширения
    mv "$f" "$DIR/$NAME/"          # перенос файла в него
    echo "Перенесён файл: $f -> $DIR/$NAME/"
    COUNT=$((COUNT + 1))
done

echo "Готово. Перенесено файлов .$EXT: $COUNT, каталог: $DIR"
```

## 5. Результаты выполнения

Проверка выполнялась в пустом каталоге-копии `lab10/demo`, в котором лежат файлы
`hello.c`, `prog1.c`, `prog2.c` и `notes.txt` (последний не подходит под расширение `.c`, и
скрипты его не должны трогать). Имя каталога вводится на запрос. После запуска показан
текущий каталог и дерево созданного каталога.

### 5.1. Windows, базовый вариант (имя каталога `result1`)

```text
=== до запуска:
hello.c
notes.txt
prog1.c
prog2.c
=== ввод: result1
Enter directory name:Copied file: hello.c
Copied file: prog1.c
Copied file: prog2.c
Done. Copied files .c: 3, directory: result1
=== после запуска, текущий каталог:
hello.c
notes.txt
prog1.c
prog2.c
result1
=== дерево каталога result1:
result1
result1/hello.c
result1/prog1.c
result1/prog2.c
```

### 5.2. Windows, усложнённый вариант (имя каталога `result2`)

```text
=== до запуска:
hello.c
notes.txt
prog1.c
prog2.c
=== ввод: result2
Enter directory name:Moved file: hello.c to result2\hello\
Moved file: prog1.c to result2\prog1\
Moved file: prog2.c to result2\prog2\
Done. Moved files .c: 3, directory: result2
=== после запуска, текущий каталог:
notes.txt
result2
=== дерево каталога result2:
result2
result2/hello
result2/hello/hello.c
result2/prog1
result2/prog1/prog1.c
result2/prog2
result2/prog2/prog2.c
```

### 5.3. Linux, базовый вариант (имя каталога `result1`)

```text
=== до запуска:
hello.c
notes.txt
prog1.c
prog2.c
=== ввод: result1
Скопирован файл: hello.c
Скопирован файл: prog1.c
Скопирован файл: prog2.c
Готово. Скопировано файлов .c: 3, каталог: result1
=== после запуска, текущий каталог:
hello.c
notes.txt
prog1.c
prog2.c
result1
=== дерево каталога result1:
result1
result1/hello.c
result1/prog1.c
result1/prog2.c
```

### 5.4. Linux, усложнённый вариант (имя каталога `result2`)

```text
=== до запуска:
hello.c
notes.txt
prog1.c
prog2.c
=== ввод: result2
Перенесён файл: hello.c -> result2/hello/
Перенесён файл: prog1.c -> result2/prog1/
Перенесён файл: prog2.c -> result2/prog2/
Готово. Перенесено файлов .c: 3, каталог: result2
=== после запуска, текущий каталог:
notes.txt
result2
=== дерево каталога result2:
result2
result2/hello
result2/hello/hello.c
result2/prog1
result2/prog1/prog1.c
result2/prog2
result2/prog2/prog2.c
```

### 5.5. Пустое имя каталога (Linux)

```text
Имя каталога не задано.
код возврата: 1
```

## 6. Объяснение результатов

1. **Базовый вариант.** Скрипт спрашивает имя каталога (`read -p` / `set /p`), создаёт его
   (`mkdir`) и в цикле по файлам `*.c` копирует их (`cp` / `copy`). Исходные файлы остаются на
   месте, а в каталоге `result1` оказываются копии трёх `.c`-файлов; `notes.txt` не
   затронут — шаблон `*.c` его не выбирает.
2. **Усложнённый вариант.** Для каждого файла имя без расширения вычисляется
   (`${f%.c}` в Linux, `%%~nf` в Windows), создаётся одноимённый подкаталог внутри
   введённого каталога и файл переносится в него (`mv` / `move`). В текущем каталоге
   остаётся только `notes.txt`, а `hello.c` оказывается в `result2/hello/hello.c` и т. д.
3. **Счётчик в цикле Windows.** Переменная `COUNT` изменяется внутри `for`, поэтому она
   читается как `!COUNT!` при включённом `EnableDelayedExpansion`; обычное `%COUNT%` показало
   бы значение до начала цикла (0). Так же читается `!NAME!`.
4. **Особенности Windows.** Файлы `.bat` написаны на английском: `cmd` читает их в кодовой
   странице OEM, и русские символы в UTF-8 ломают разбор (в этом пришлось убедиться при
   отладке); в Linux такого ограничения нет. Комментарии вынесены из блока `for (…)`: скобки
   в тексте комментария внутри блока могут привести к ошибке разбора.
5. **Обработка ошибок.** Пустое имя каталога отвергается (код возврата 1, 5.5). Если файлов
   с нужным расширением нет, в Linux шаблон `*.c` остаётся строкой, поэтому в цикле стоит
   проверка `[ -f "$f" ] || continue`.

## 7. Контрольные вопросы

В методических указаниях к лабораторной работе №10 контрольных вопросов нет.
