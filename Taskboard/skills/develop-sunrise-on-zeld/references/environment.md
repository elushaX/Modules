# Окружение и сборка

## Топология

    локальная машина -> nmotin@supermarket-wm -> zeld-lpor-nmotin

Если supermarket-wm недоступен, использовать unitload-pc как jump-host после проверки доступа.

Основная зона работы на zeld:

    ~/work/sunrise/clod

Не считать существующие каталоги свободными. На хосте одновременно работают несколько агентов под одним Unix-пользователем.

## Изоляция

Каждая задача обязана иметь:

- уникальный worktree;
- уникальную ветку;
- собственный build-каталог внутри worktree;
- собственный Jenkins state-файл внутри worktree;
- уникальный каталог deploy на платах.

Безопасный suffix: ${USER}-YYYYmmdd-HHMMSS-nanoseconds-PID.

Не удалять и не переиспользовать неизвестные worktrees. Перед удалением своего worktree завершить процессы, освободить hardware resources и проверить абсолютный путь.

## SDK

Стандартный SDK:

    sdk="$HOME/.local/Elvees-MCom03-SDR-SDK"
    export MCOM03_ARM_BIN_ROOT="$sdk/ARM-BIN-ROOT"
    export MCOM03_ELCORE50_BIN_ROOT="$sdk/ELCORE50-BIN-ROOT"
    export MCOM03_ELCORE50_CMAKE_TOOLCHAIN_FILE="$MCOM03_ELCORE50_BIN_ROOT/share/cmake/elcore50_toolchain.cmake"
    export STAGING_DIR="$MCOM03_ARM_BIN_ROOT"

В неинтерактивном SSH эти переменные не появляются автоматически. Экспортировать их в каждом build-script.

SKIF-MP-BIN-ROOT использовать только для конфигураций mcom03+fpga, не для обычной MCom03 ARM-сборки.

## Presets

Одинаковые release configure presets в modules и runtime:

    Linux-Release
    Arm-Release
    Elcore50-Release

Нативная последовательность:

    cmake --preset Linux-Release
    cmake --build build/Linux-Release -j8
    ctest --test-dir build/Linux-Release --output-on-failure

    cmake --preset Arm-Release
    cmake --build build/Arm-Release -j8

    cmake --preset Elcore50-Release
    cmake --build build/Elcore50-Release -j8

Запускать из корня соответствующего repository/worktree. build-native.sh выполняет эту последовательность и устанавливает SDK environment.

## Submodules и MR

modules использует как минимум extern/runtime и другие submodules. После merge или изменения gitlink выполнять:

    git submodule update --init --recursive

Для MR получать точную head ref, а не полагаться на локальную ветку:

    git fetch origin refs/merge-requests/NNN/head:refs/remotes/origin/mr-NNN

После долгой работы повторно сверять MR heads: они могли измениться во время сборки и стендовых тестов.

## Проверка

Минимум для интеграционной задачи:

- Linux configure/build;
- полный Linux CTest;
- ARM configure/build;
- Elcore50 configure/build;
- профильные hardware apps на зарезервированном стенде;
- повторная проверка после последнего изменения исходников.

Для долгих detached-прогонов использовать skill long-run-notify, явный final marker и отдельный log.
