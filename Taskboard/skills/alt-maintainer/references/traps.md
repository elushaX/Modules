# Каталог граблей

Каждая запись: симптом → причина → лечение. Проверено на живых сборках.

---

## rpmbuild

**`process-lto: ... contains __gnu_lto_slim only`**
LTO Alt делает «тонкие» объектники, статический архив становится нечитаемым для проверок.
→ `%{?optflags_lto:%global optflags_lto %optflags_lto -ffat-lto-objects}`

**Файлы установились в дерево исходников, а не в `%buildroot`**
`-DLIB_INSTALL_DIR=lib64` без типа: cmake считает переменную `CACHE PATH` и достраивает её до
абсолютного пути относительно текущего каталога.
→ `-DLIB_INSTALL_DIR:STRING=%_lib`

**`verify-elf` / `find-requires` падают на файлах Elcore50**
DSP-ELF имеет `arch 0x2718`, объектники — `.obj`.
→ `%add_verify_elf_skiplist`, `%add_findreq_skiplist`, `%add_findprov_skiplist` на
`%_datadir/elcore50/sdk/*`

**Spec не подхватывается**
Имя файла не совпадает с `%name` (`%setup -n %name` не находит каталог).
→ переименовать файл spec.

**`git add` игнорирует spec**
`.gitignore`, сгенерированный gitignore.io (репозиторий fast-dds), содержит `*.spec`.
→ `git add -f`, причину — в сообщение коммита.

---

## build-scripts

**`PACKAGES2_EXIT` / «pkgfile not found» внутри контейнера**
Контейнер ищет файл через `find $REPO_ROOT/../ -name $(basename ...)` — путь вне дерева репозитория
не находится.
→ класть список в `config/packages/`, даже временный.

**`BuildRequires` собственного пакета не резолвится в том же прогоне**
`container_build::add_local_repo_to_altlist` вызывался в конце итерации цикла, поэтому первый
пакет прогона локального репозитория не видел.
→ вызов перенесён **до** цикла в `docker/pkg_build_entrypoint.sh` (коммит `bd619a5`). Не двигать обратно.

**Стадия `check-lists` в build_distro.sh падает со списком своих пакетов**
`--repos-policy` по умолчанию `remote`, образ собирается из зеркала.
→ `./build_distro.sh --repos-policy=local`

**`WORKSPACE_ROOT` не задан** — workspace создаётся в текущем каталоге.
→ `export WORKSPACE_ROOT=$HOME/buildalt`

---

## SDK / sysroot

**Пакет распакован «наполовину»: каталоги есть, библиотек нет**
usr-merge в образе: файл лежит как `./usr/lib64/libcrypto.so.3`, база rpm показывает
`/lib64/libcrypto.so.3`.
→ два прохода tar в `sdk::populate_sysroot`; второй берёт `./usr/...`-пути и раскладывает их с
`--transform 's,^\./usr/,./,'` — именно туда ведут симлинки из `-devel` (в sysroot Bootlin
`usr/lib64 -> lib` и `lib64 -> lib`).

**Заголовки `linux/*` не распаковались**
`/usr/include/linux` — alternatives-симлинк на `/usr/include/linux-default/include/linux`.
→ запрашивать оба пути (`sed 'p; s|^\./usr/include/linux/|...|'`).

**sed «съел» альтернативу**
В `s|...|...|` последовательность `\|` — это литеральная вертикальная черта, а не оператор `\(a\|b\)`.
→ сменить разделитель: `s,^/\(bin\|sbin\|lib64\|lib\)/,./usr/\1/,`

**`find_package` не находит Fast DDS / Fast CDR при наличии пакета в sysroot**
Проекты eProsima кладут файлы пакета CMake в `share/<name>/cmake`, не в `lib/cmake`.
→ в тулчейн-файле:
```cmake
set(CMAKE_PREFIX_PATH
  "${CMAKE_SYSROOT}/usr/lib64/cmake"
  "${CMAKE_SYSROOT}/usr/lib/cmake"
  # Проекты eProsima держат файлы своего пакета CMake в share, а не в lib/cmake.
  "${CMAKE_SYSROOT}/usr"
)
```

**`undefined reference to compress / inflateEnd`**
`libcrypto` тянет zlib, а zlib в sysroot не попал.
→ `zlib` + `zlib-devel;zlib` в `config/sdk/sdk.pkgs`.

**Экспортированная цель ссылается на `/usr/lib64/...`, в sysroot её нет**
См. ловушку `%cmake` в `spec-templates.md` §2.

**В SDK остались абсолютные пути разработчика**
`esdr_runtime_export.cmake` и `AdfeApiConfig.cmake` из SDR-SDK. Это ожидаемо — их правит
`relocate-sdk.sh` самого SDK. Задокументировано в `docs/sdk.md`.

---

## Кросс-сборка runtime / modules

**Нет `args.hxx`, `fkYAML/node.hpp`, `plog/Log.h`**
Это сабмодули, а не пакеты.
→ `git submodule update --init --depth 1`

**`NoSuchMethodError: java.lang.String.isBlank()`**
fastddsgen требует Java 11+, на supermarket-wm по умолчанию Java 8.
→ `export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-...` и `PATH=$JAVA_HOME/bin:$PATH`

**Агент не компилируется с дистрибутивным `libspdlog-devel`**
Он собран против внешнего fmt v12, требующего явных специализаций `formatter`
(`type_is_unformattable_for`).
→ пакет `elvees-spdlog-devel` (форк с bundled fmt, `Conflicts: libspdlog-devel`).

**`mcom03_ipc` не собирается в Debug**
Три независимых бага в `assert`-ах: неквалифицированные `MCOM03_IPC_MASTER_*_BUFFER_NUM`,
`.size` вместо `.sizeInBytes`, и `ctx->writable_zones` — поля, которого в структуре никогда не было.
Первые два исправлены, третий не восстановим по исходникам.
→ собирать `-DCMAKE_BUILD_TYPE=Release` (NDEBUG), как и все пресеты проекта; о третьем баге
сообщить владельцам репозитория, а не выдумывать поле.

---

## Методика проверки

**Не глобить имена rpm.** В локальном репозитории лежат прошлые версии, и `*-devel-*.rpm`
подсунет старый файл — проверишь несобранное и отчитаешься об успехе зря. Всегда пинить точную
версию из вывода сборки.

**Считать готовым только по конечному потребителю.** Критерий для этого проекта:
```bash
cmake --preset Arm-Alt-Release      && cmake --build build/Arm-Alt-Release      # BUILD=0
cmake --preset Elcore50-Alt-Release && cmake --build build/Elcore50-Alt-Release # BUILD=0
grep -rn "Elvees-MCom03-SDR-SDK" build/*/CMakeFiles/*/flags.make build/*/CMakeFiles/*/link.txt
# ^ должно быть пусто для Arm
```
