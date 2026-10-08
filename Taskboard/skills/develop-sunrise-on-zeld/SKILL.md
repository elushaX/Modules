---
name: develop-sunrise-on-zeld
description: "Нативная разработка Sunrise/SRISE на zeld-lpor-nmotin через supermarket-wm: создание обязательного уникального git worktree и ветки, настройка MCom03 SDK, сборка и тестирование runtime/modules для Linux, ARM и Elcore50, доставка приложений на платы, резервирование Jenkins lockable resources и безопасная диагностика стенда. Использовать при просьбах собрать или проверить runtime/modules на zeld, интегрировать ветки/MR в изолированной папке, запустить YAML-приложение на MCom03 или работать со стендом без Docker."
disable-model-invocation: true
---

# Работа с Sunrise на zeld

Выполнять инженерные команды на zeld-lpor-nmotin через `nmotin@supermarket-wm`. Не использовать Docker: установленный MCom03 SDK обеспечивает нативную сборку.

## Обязательный порядок

1. Прочитать /home/nmotin/work/claude/.codex/memory/MEMORY.md и только релевантные карточки.
2. Подключаться через scripts/zeld-run.sh.
3. Всегда создать новую уникальную папку и ветку через scripts/new-worktree.sh. Не собирать и не редактировать общий clone или чужой worktree.
4. Обновить нужные refs/MR в source clone до создания worktree.
5. Инициализировать submodules внутри нового worktree.
6. Собирать через scripts/build-native.sh; явно выбрать presets.
7. Перед аппаратными командами зарезервировать все требуемые ресурсы через scripts/jenkins-resource.sh.
8. Проверять результат явным exit code, marker-файлом и ожидаемыми артефактами. Отсутствие процесса не считать успехом.
9. После тестов завершить приложения через TERM, перезагрузить использованные платы и освободить только свои ресурсы.

## Правка файлов в удалённом worktree

Локальный `apply_patch` видит только локальную файловую систему и не может менять абсолютные
пути worktree на `zeld-lpor-nmotin`. Для удалённых файлов передавать diff через
`scripts/zeld-run.sh --stdin` и применять его командой `patch` уже на zeld. Не передавать
удалённые абсолютные пути локальному `apply_patch`.

## Быстрый старт

Войти интерактивно:

    scripts/zeld-run.sh --shell

Создать изолированную ветку/worktree на zeld:

    scripts/zeld-run.sh --script scripts/new-worktree.sh ~/work/sunrise/clod/modules ofdm-yaml origin/main

Собрать все release-платформы:

    scripts/zeld-run.sh --script scripts/build-native.sh --tests /path/to/unique-worktree all-release

Передать многострочный скрипт на zeld без вложенного quoting:

    scripts/zeld-run.sh --stdin <<'REMOTE'
    set -e
    cd /path/to/unique-worktree
    git status --short
    REMOTE

## Секреты и публичные артефакты

- Читать ~/git.token, ~/jira.token, ~/confluence.token и ~/jenkins.token только внутри команды на zeld.
- Никогда не печатать и не переносить токены локально.
- Не помещать секреты в URL, patches, logs, commits, MR и memory.
- Не добавлять attribution агента в commits и MR.

## Подробности

- Окружение, presets и сборка: [references/environment.md](references/environment.md).
- Jenkins, платы и диагностика: [references/stand.md](references/stand.md).
