#!/bin/bash
# Шаблон скрипта-исполнителя для долгой работы на сборочном хосте.
#
# Запуск detached (иначе умрёт вместе с ssh-сессией):
#   setsid nohup /tmp/chain.sh > /tmp/task-logs/chain-run.log 2>&1 < /dev/null &
#
# Главное требование: последняя строка лога - маркер исхода. По нему, а не по
# наличию процесса, сторож определяет, что работа кончилась (см. SKILL.md, п.3).
set -Eeuo pipefail

# Всё окружение задаётся явно: в detached-процессе нет ничего из вашего
# интерактивного профиля.
export WORKSPACE_ROOT="$HOME/buildalt"
cd "$WORKSPACE_ROOT/build-scripts"

LOGDIR=/tmp/task-logs
mkdir -p "$LOGDIR"

# Маркер провала пишется в любом случае, в том числе при падении по set -e:
# без этого сторож не отличит крах от «ещё работает».
trap 'echo "ЗАДАЧА УПАЛА rc=$? $(date)"' ERR

echo "=== СТАДИЯ 1: старт $(date) ==="
./build_packages.sh --pkgfile=config/packages/alt.pkgs
echo "=== СТАДИЯ 1: готова $(date) ==="

# Проверять исход предыдущей стадии явно, если она запускалась не под set -e
# (например, через обёртку, которая глотает код возврата).
echo "=== СТАДИЯ 2: старт $(date) ==="
./build_distro.sh --repos-policy=local
echo "=== СТАДИЯ 2: готова $(date) ==="

echo "ВСЁ ГОТОВО rc=0"
