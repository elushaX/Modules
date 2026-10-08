---
name: alt-maintainer
description: Сопровождение сборки дистрибутива Alt Linux p11 для MCom03 (проект Восход/SRISE) — написание и правка spec-файлов для кросс-сборки под aarch64 и DSP Elcore50, ведение списков build-scripts (alt.pkgs, sdk.pkgs), сборка пакетов/образа/SDK на zeld-lpor-nmotin, диагностика ошибок rpmbuild и «пакет собрался, но не находится в sysroot». Использовать, когда просят «упакуй библиотеку в rpm под Alt», «добавь пакет в дистрибутив/SDK», «собери образ», «собери SDK», «почему не линкуется под Alt», «перенеси зависимость из SDR-SDK в пакет».
disable-model-invocation: true
---

# Мейнтейнер Alt Linux (MCom03, Восход)

Задача скилла — превращать библиотеки ELVEES в rpm-пакеты Alt p11 под aarch64 и Elcore50,
собирать из них образ и SDK и доводить до состояния «`cmake --preset ...-Alt-Release` собирается
без единой ссылки на SDR-SDK разработчика».

## Окружение

**Всё, что относится к сборке Alt, выполняется только на `zeld-lpor-nmotin`** (там смонтирован
репозиторий Alt и живёт docker). Локальной сети нет, ходить через цепочку:

```bash
ssh supermarket-wm "ssh zeld-lpor-nmotin '...'"   # при недоступности — unitload-pc
```

Кросс-сборка runtime/modules против готового SDK делается на `supermarket-wm` (там же лежат
распакованные корни SDK).

Токены на хосте (читать из файла, не вставлять в текст команды): `~/git.token`, `~/jira.token`,
`~/confluence.token`, `~/jenkins.token`.

Рабочие каталоги на zeld-lpor-nmotin:
- `~/buildalt/build-scripts` — сборочная инфраструктура (ветка `feat/SRISE-408-build-sdk`)
- `~/buildalt/buildalt_tmp` — workspace: `source/`, `build/`, `apt-rpm-repo/`, `mkimage-profiles/`, `sdk-work/`
- `~/buildalt/buildalt_tmp/build/images` — образы, `build/sdk` — корни SDK

**В коммитах, MR, Jira и Confluence не упоминать AI-агента** — ни `Co-Authored-By`, ни
`Claude-Session`, ни Codex/OpenAI, ни «Generated with». Это жёсткое требование.

Проверенные сведения окружения и диагностики находятся в
`/home/nmotin/work/claude/.codex/memory/`. Перед работой прочитать индекс `MEMORY.md`, затем
только относящиеся к задаче карточки.

## Конвейер

Четыре скрипта, запускать из корня build-scripts с обязательным `WORKSPACE_ROOT`:

```bash
cd ~/buildalt/build-scripts
export WORKSPACE_ROOT=$HOME/buildalt

./build_packages.sh --pkgfile=config/packages/alt.pkgs \
                    --source-policy=clone|reuse --rebuild-policy=full|failed_only
./build_kernel.sh
./build_distro.sh  --repos-policy=local        # ВАЖНО: по умолчанию remote!
./build_sdk.sh     [--image=<path>] [--no-archive]
```

- `--repos-policy` по умолчанию `remote` — берёт пакеты с зеркала. Свежесобранные локальные
  пакеты попадут в образ **только** с `--repos-policy=local`. Симптом ошибки: стадия
  `check-lists` падает со списком имён своих пакетов.
- `--pkgfile=PATH` должен лежать **внутри дерева репозитория build-scripts**: контейнер ищет
  файл через `find $REPO_ROOT/../ -name $(basename ...)`. Для пробных прогонов класть временный
  список в `config/packages/`, а не в `/tmp`.
- Сборка идёт в контейнере `alt-mcom03-rpm-builder:p11`, `TARGET_PLATFORM=linux/arm64`.
- Все настройки — `config/global_build_config.sh`, переопределяются переменными окружения
  (`: "${VAR:=default}"`), поэтому `MKIMAGE_PROFILES_BRANCH=feat/... ./build_distro.sh` работает.
  По умолчанию ветка mkimage-profiles — `dev`.

### Форматы списков

`config/packages/alt.pkgs` — что собирать, **порядок = порядок зависимостей**:
```
<имя>;rpmbuild;<git-url>;<ветка|DEFAULT_BRANCH>;LAST_COMMIT_ON_BRANCH
```

`config/sdk/sdk.pkgs` — что класть в sysroot SDK:
```
<пакет>              # версия берётся из образа
<пакет>;<донор>      # версию взять как у пакета-донора (обычно -devel;<runtime-пакет>)
<пакет>;*            # любая доступная версия (пакета нет в образе — только в локальном репо)
```
Правило: `-devel` для библиотеки, которая есть в образе, — `;<runtime-пакет>`; чисто сборочные
пакеты (headers-only, статические, Elcore50) — `;*`.

`pkg.in/lists/mcom03` в **mkimage-profiles** — что попадает в сам образ (только runtime-пакеты).

### Проверка результата

```bash
# что реально внутри пакета
rpm -qlp <pkg>.rpm
# экспортированные пути в CMake-конфиге — не должно быть /usr/lib64 и путей разработчика
rpm2cpio <pkg>.rpm | cpio -it | grep cmake
grep -rn "_IMPORT_PREFIX\|/home/" <распакованный конфиг>
# в собранном runtime/modules не должно остаться SDR-SDK
grep -rn "Elvees-MCom03-SDR-SDK" build/*/CMakeFiles/*/flags.make build/*/CMakeFiles/*/link.txt
```
При проверке rpm **никогда не глобить имена** (`*-devel-*.rpm`) — в локальном репозитории лежат
старые версии, и проверишь несобранное. Пинить точную версию из вывода сборки.

## Написание spec для Alt p11

Полные шаблоны — `references/spec-templates.md`. Ключевые правила:

- **Имя файла spec обязано совпадать с `%name`** (`%setup -n %name` + `Source: %name-%release_stamp.tar.gz`).
- **Первые 256 байт файла — только ASCII.** rpm отличает spec от прочих файлов по печатности
  первых 256 байт, а в локали C кириллица непечатна. Комментарий на русском выше преамбулы даёт
  `does not appear to be a specfile` — при том что `rpmspec -P` под root'ом (с выставленной локалью)
  тот же файл разбирает без ошибок. У всех рабочих spec проекта кириллица начинается после 330-го
  байта; пояснения на русском писать ниже `Name:`.
- `%define release_stamp` автоматически переписывается build-scripts из git — не трогать руками.
- `BuildRequires(pre): rpm-macros-cmake`, дальше `%cmake` / `%cmake_build` / `%cmake_install`.
- `%_lib` = `lib64` (относительный), `%_libdir` = `/usr/lib64` (абсолютный). Для install-путей
  внутри пакета нужен относительный.
- Опции — через `%def_with` / `%if_with`.
- Проверки rpm глушить точечно: `%add_verify_elf_skiplist`, `%add_findreq_skiplist`,
  `%add_findprov_skiplist` (обязательно для DSP-артефактов Elcore50).
- **Симлинк на файл, которого нет в пакете, порождает Requires на его цель.** Симлинк
  `/opt/esdr_platform/board_type -> /run/esdr/board_type` дал `Requires: /run/esdr/board_type`,
  а этот файл создаётся сервисом при загрузке — пакет становится неустанавливаемым.
  Лечится `%add_findreq_skiplist <каталог>/*`.
- **Скрипты в `/etc/profile.d` ставить с правами 0755.** `/etc/profile` в ALT обходит
  `/etc/profile.d/*.sh` с условием `-x`; файл с 0644 молча не подключается, и переменные
  не попадают в интерактивные сессии.
- Параллельность — `%_smp_mflags`.

### Три ловушки, на которых спотыкаешься каждый раз

1. **LTO и статические библиотеки.** `process-lto` падает с `contains __gnu_lto_slim only`.
   Лечится в spec:
   ```spec
   %{?optflags_lto:%global optflags_lto %optflags_lto -ffat-lto-objects}
   ```

2. **`%cmake` подсовывает абсолютные KDE-переменные.** Макрос Alt (`/usr/lib/rpm/macros.d/cmake`)
   передаёт `-DINCLUDE_INSTALL_DIR:PATH=%_includedir`, `-DLIB_INSTALL_DIR:PATH=%_libdir`,
   `-DSHARE_INSTALL_PREFIX:PATH=%_datadir`, `-DLIB_DESTINATION=%_lib`, `-DLIB_SUFFIX=%_libsuff`.
   Проекты eProsima (Fast DDS, Fast CDR, Micro-XRCE, micro-CDR, agent) используют ровно эти имена
   переменных → в экспортированных целях CMake оказывается абсолютный `/usr/lib64/...`, и в
   sysroot цель не находится. Перебивать относительными:
   ```spec
   %cmake \
       -DLIB_INSTALL_DIR:STRING=%_lib \
       -DINCLUDE_INSTALL_DIR:STRING=include \
       -DDATA_INSTALL_DIR:STRING=share \
       ...
   ```
   **Тип `STRING` обязателен**: значение `CACHE PATH`, переданное из командной строки, cmake
   достраивает до абсолютного относительно текущего каталога — и установка уезжает в дерево
   исходников.

3. **`.gitignore` из gitignore.io прячет `*.spec`** (репозиторий fast-dds). Добавлять
   `git add -f`, причину писать в сообщении коммита.

### Подпакеты Elcore50 (DSP)

Собираются вторым проходом в том же spec, тулчейн монтируется в контейнер:

```spec
%define _ec_prefix %{?ec_prefix}%{?!ec_prefix:/opt/elcore50-toolchain-clang14}
%define _elcore50_sdk %_datadir/elcore50/sdk
%add_verify_elf_skiplist %_elcore50_sdk/*

export MCOM03_ELCORE50_BIN_ROOT=%_ec_prefix
cmake -B build-elcore50 \
    -DCMAKE_TOOLCHAIN_FILE=%_ec_prefix/share/cmake/elcore50_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%_elcore50_sdk \
    -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_INCLUDEDIR=include \
    -DCMAKE_INSTALL_DATAROOTDIR=share
cmake --build build-elcore50 -- %_smp_mflags
...
DESTDIR=%buildroot cmake --install build-elcore50
```

ELF DSP имеет `arch 0x2718`, объектники — `.obj`; штатные проверки rpm их не понимают, отсюда
skiplist'ы. Имя подпакета — `<lib>-elcore50-devel`, в `sdk.pkgs` ставится с `;*`.

### Прошивка RISC1 (MIPS32)

Пакет `sdr-subsystem-firmware` (репозиторий `openwrt-infrastructure/elv-freertos-unsecure-ipc`,
ветка `feat/SRISE-408-alt-package`) даёт `/opt/esdr_platform/risc2.elf` и `sdr-subsystem.service`.
Собирается так же, как Elcore50-части: кросс-компилятор MIPS32 берётся из SDR-SDK
(`MCOM03_RISC1_BIN_ROOT`), монтируется в `/opt/risc1-toolchain-sdr-sdk` и приходит в spec через
`--define risc1_prefix` (`docker::elcore50_toolchain_run_opts` + `container_build.sh`).

```spec
%define _risc1_prefix %{?risc1_prefix}%{?!risc1_prefix:/opt/risc1-toolchain-sdr-sdk}
%brp_strip_none %_esdr_platform_dir/*      # прошивку класть в пакет как есть
cmake -B build-risc1 -DCMAKE_TOOLCHAIN_FILE=%_risc1_prefix/share/cmake/mips_toolchain.cmake ...
```

`%cmake` здесь **нельзя**: он подставляет флаги оптимизации хостового aarch64. Пресет `mips32`
из репозитория разворачивать вручную, обнуляя `CMAKE_{C,CXX,ASM,EXE_LINKER}_FLAGS`.

Подмодули: `workspace::update_submodules` инициализирует **только верхний уровень** —
вложенные подмодули (`no-OS/libraries/azure/...`) ведут на github, которого с хоста нет,
и `--recursive` там виснет.

## Сборка SDK

`build_sdk.sh` делает два корня: `ARM-ALT-BIN-ROOT` (тулчейн Bootlin + sysroot из образа) и
`ELCORE50-ALT-BIN-ROOT`.

Elcore50-корень собирается так: копия `MCOM03_ELCORE50_BIN_ROOT` (SDR-SDK разработчика) →
`sdk::drop_elcore50_libraries` выкидывает то, что теперь приходит пакетами (список
`SDK_ELCORE50_DROP` в `global_build_config.sh`) → `sdk::add_elcore50_libraries` накладывает
`include/ lib/ share/` из пакетов (`SDK_ELCORE50_SDK_DIR=usr/share/elcore50/sdk`).

Из SDR-SDK осознанно остаётся: компилятор/libc++, `DspLib`, `DataStream`, `FreertosKernel`,
`ElcoreRuntimeFreertos`, `liquid`, `AdfeApi`. Два файла (`esdr_runtime_export.cmake`,
`AdfeApiConfig.cmake`) всё ещё несут абсолютный путь разработчика — это чинит собственный
`relocate-sdk.sh` из SDK, задокументировано в `docs/sdk.md`.

### usr-merge — главная засада sysroot

В образе действует usr-merge: `/lib64`, `/bin` — симлинки в `/usr`, файл лежит по пути
`./usr/lib64/libcrypto.so.3`, а база rpm показывает `/lib64/libcrypto.so.3`. tar по списку из базы
молча ничего не находит: каталоги появляются, библиотек нет. В `sdk::populate_sysroot` это решено
двумя проходами tar: второй распаковывает `./usr/...`-пути с `--transform 's,^\./usr/,./,'`, потому
что в sysroot Bootlin `usr/lib64 -> lib` и симлинки из `-devel` ведут именно туда.

Отдельно: `/usr/include/linux` в Alt — alternatives-симлинк на `/usr/include/linux-default/include/linux`,
поэтому запрашиваются оба пути.

**Про sed:** в `s|...|...|` конструкция `\|` читается как *литеральная вертикальная черта*, а не
альтернатива. Для групп вида `\(bin\|lib64\)` менять разделитель на `,`.

### Диагностика «пакет собрался, но линковка падает»

Порядок проверки, снизу вверх:
1. Файл вообще попал в sysroot? (`find $SYSROOT -name 'lib*.so*'`) — если нет, это usr-merge.
2. Пакет есть в `sdk.pkgs`? Транзитивные зависимости тоже нужны: `fastdds-config.cmake` тянет
   `foonathan_memory` и `TinyXML2`; `libcrypto` тянет `zlib`. Симптом — `undefined reference to
   compress/inflateEnd`.
3. CMake-конфиг экспортирует абсолютный `/usr/lib64`? → ловушка №2 выше.
4. Проекты eProsima кладут файлы пакета CMake в `share/<name>/cmake`, а не `lib/cmake` — в
   тулчейне нужен `CMAKE_PREFIX_PATH` с `${CMAKE_SYSROOT}/usr`.

## Локальный репозиторий пакетов

`docker/pkg_build_entrypoint.sh` подключает локальный репозиторий (`add_local_repo_to_altlist`)
**до** цикла сборки — иначе первый пакет прогона не видит `BuildRequires`, собранные в этом же
прогоне. Если правишь entrypoint — не двигай этот вызов обратно в конец итерации.

## Пресеты для кросс-сборки под Alt

В `runtime` и `modules` есть пресеты `Arm-Alt-*` и `Elcore50-Alt-*`. Окружение для сборки на
supermarket-wm:

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-17.0.6.0.9-0.3.ea.el8.x86_64   # fastddsgen требует Java 11+
export PATH=$JAVA_HOME/bin:$HOME/work/sunrise/fastddsgen-pkg/bin:$PATH
export MCOM03_ARM_ALT_BIN_ROOT=$HOME/work/sunrise/alt-sdk-fresh/ARM-ALT-BIN-ROOT
export MCOM03_ELCORE50_ALT_BIN_ROOT=$HOME/work/sunrise/alt-sdk-fresh/ELCORE50-ALT-BIN-ROOT
export MCOM03_ELCORE50_CMAKE_TOOLCHAIN_FILE=$MCOM03_ELCORE50_ALT_BIN_ROOT/share/cmake/elcore50_toolchain.cmake
```

Перед первой сборкой: `git submodule update --init --depth 1`. Отсутствие `args.hxx`,
`fkYAML/node.hpp`, `plog/Log.h` — это неинициализированные сабмодули, а **не** нехватка пакетов.

## Чего не делать

- Не «чинить» чужие assert-only баги молча: если библиотека не собирается в Debug из-за
  несуществующего поля структуры, собрать в Release (как во всех пресетах проекта) и **сообщить
  владельцам репозитория**, а не выдумывать поле.
- Не считать сборку проверенной, пока не собран конечный потребитель (runtime/modules) целиком.
- Не полагаться на глоб при проверке rpm (см. выше).
