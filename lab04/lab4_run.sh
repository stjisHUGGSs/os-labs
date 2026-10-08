#!/bin/bash
# ЛР №4, усложнённый вариант: автоматический запуск трёх экземпляров
# lab4.out с интервалом 3 секунды между ними.
#
# Программа рисует сообщения по абсолютным координатам экрана, поэтому каждому
# экземпляру нужна своя консоль. Роль «виртуальных консолей» играют окна tmux:
# переключение между ними — Ctrl-b n / Ctrl-b p, отключиться — Ctrl-b d.
# Если tmux нет, экземпляры запускаются в фоне, а вывод идёт в файлы lab4_N.log.

COUNT=3      # сколько экземпляров
DELAY=3      # пауза между запусками, секунд (по заданию 3 или 4)
SESSION=lab4

cd "$(dirname "$0")" || exit 1
[ -x ./lab4.out ] || gcc lab4.c -o lab4.out -Wall || exit 1   # собрать, если ещё не собрано

if command -v tmux >/dev/null 2>&1; then
    tmux kill-session -t "$SESSION" 2>/dev/null            # убрать сеанс от прошлого запуска
    tmux new-session -d -s "$SESSION" -n inst1 -x 120 -y 36 # сеанс; в нём окно 1
    for i in $(seq 1 "$COUNT"); do
        [ "$i" -gt 1 ] && tmux new-window -t "$SESSION" -n "inst$i"   # окно для экземпляра i
        tmux send-keys -t "$SESSION:inst$i" "./lab4.out" Enter        # запустить в этом окне
        echo "Экземпляр $i запущен в $(date +%T)"
        [ "$i" -lt "$COUNT" ] && sleep "$DELAY"                       # пауза до следующего
    done
    echo "Все экземпляры запущены (tmux, сеанс $SESSION)."
    [ -t 0 ] && tmux attach -t "$SESSION"                  # показать, если запущено из терминала
else
    for i in $(seq 1 "$COUNT"); do
        ./lab4.out > "lab4_$i.log" 2>&1 &                  # фон, вывод в файл
        echo "Экземпляр $i запущен в $(date +%T), вывод: lab4_$i.log"
        [ "$i" -lt "$COUNT" ] && sleep "$DELAY"
    done
    wait
fi
