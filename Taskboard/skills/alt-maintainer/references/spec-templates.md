# Шаблоны spec-файлов (Alt p11, кросс-сборка под MCom03)

Все шаблоны проверены на реальных пакетах проекта. Имя файла spec = `%name`.

---

## 1. Библиотека для Arm (CMake, -devel)

Типовой случай: `dmrlib-devel`, `cxx17to14compatibility-devel`, `mcom03-ipc-devel`.

```spec
%define release_stamp 1
# LTO ломает статические архивы: process-lto ругается
# "contains __gnu_lto_slim only".
%{?optflags_lto:%global optflags_lto %optflags_lto -ffat-lto-objects}

Name:      dmrlib-devel
Version:   1.0.0
Release:   alt%release_stamp

Summary:   DMR library headers and static library
License:   Proprietary
Group:     Development/C++
Url:       https://git.elvees.com/dis/libs/dmrlib

Source:    %name-%release_stamp.tar.gz

BuildRequires(pre): rpm-macros-cmake
BuildRequires: cmake gcc-c++

%description
Заголовки и статическая библиотека DMR.

%prep
%setup -n %name

%build
%cmake \
    -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%_includedir/*
%_libdir/*.a
%_libdir/cmake/*

%changelog
```

Замечания:
- `%setup -n %name` — потому что build-scripts формируют тарбол как `%name-%release_stamp.tar.gz`
  с корневым каталогом `%name`.
- Если проект ставит только заголовки — `BuildArch: noarch` не ставить, если есть DSP-подпакет.

---

## 2. Проект eProsima (Fast DDS и родня): относительные install-пути

Обязательно для `libfastcdr`, `libfastdds`, `foonathan-memory-devel`, `microcdr-devel`,
`microxrcedds-client-devel`, `libmicroxrcedds-agent`.

```spec
%build
# Пути установки задаются относительными. Макрос %%cmake передаёт их абсолютными
# (-DLIB_INSTALL_DIR:PATH=%%_libdir), а проекты eProsima используют ровно эти имена
# переменных - и тогда в экспортированных целях CMake оказывается /usr/lib64, а не
# путь относительно корня пакета. В sysroot такая цель не находится.
#
# Тип STRING обязателен: значение типа PATH cmake достраивает до абсолютного от
# текущего каталога.
%cmake \
    -DLIB_INSTALL_DIR:STRING=%_lib \
    -DINCLUDE_INSTALL_DIR:STRING=include \
    -DDATA_INSTALL_DIR:STRING=share \
    -DCMAKE_BUILD_TYPE=Release
%cmake_build
```

Проверка после сборки — в экспортированном конфиге должно быть `${_IMPORT_PREFIX}/lib64/...`:

```bash
rpm2cpio libfastdds-<точная версия>.rpm | cpio -i --to-stdout '*/fastdds-targets*.cmake' \
  | grep _IMPORT_PREFIX
```

Дополнительно:
- `micro-xrce-dds-client` не конфигурируется без `-DTARGET_PLATFORM=<...>`: в его CMakeLists
  стоит `if(${TARGET_PLATFORM} STREQUAL ...)` без кавычек, пустое значение ломает синтаксис.
- Ему же нужен `-DUCLIENT_SUPERBUILD=OFF`, иначе ExternalProject собирает собственный micro-cdr.
- `libfastdds-devel` обязан требовать `foonathan_memory` и `TinyXML2` — их тянет
  `fastdds-config.cmake`.
- Агент (`micro-xrce-dds-agent`) — разделяемая библиотека, всё, что в неё линкуется статикой,
  нужно собирать с `-DCMAKE_POSITION_INDEPENDENT_CODE=ON`. BuildRequires: OpenSSL и tinyxml2.

---

## 3. Подпакет Elcore50 (DSP) внутри spec для Arm

```spec
%define _ec_prefix %{?ec_prefix}%{?!ec_prefix:/opt/elcore50-toolchain-clang14}
%define _elcore50_sdk %_datadir/elcore50/sdk

# Артефакты DSP (arch 0x2718, объектники .obj) штатные проверки rpm не понимают.
%add_verify_elf_skiplist %_elcore50_sdk/*
%add_findreq_skiplist    %_elcore50_sdk/*
%add_findprov_skiplist   %_elcore50_sdk/*

%package -n dmrlib-elcore50-devel
Summary: DMR library for Elcore50 DSP
Group:   Development/C++

%description -n dmrlib-elcore50-devel
Сборка библиотеки для DSP Elcore50.

%build
# ... сборка для Arm ...

export MCOM03_ELCORE50_BIN_ROOT=%_ec_prefix
cmake -B build-elcore50 \
    -DCMAKE_TOOLCHAIN_FILE=%_ec_prefix/share/cmake/elcore50_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%_elcore50_sdk \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DCMAKE_INSTALL_INCLUDEDIR=include \
    -DCMAKE_INSTALL_DATAROOTDIR=share
cmake --build build-elcore50 -- %_smp_mflags

%install
%cmake_install
DESTDIR=%buildroot cmake --install build-elcore50

%files -n dmrlib-elcore50-devel
%_elcore50_sdk/*
```

Тулчейн внутрь контейнера монтируется build-scripts:
`PKG_ELCORE50_TOOLCHAIN_ROOT` (= `MCOM03_ELCORE50_BIN_ROOT` хоста) →
`PKG_ELCORE50_TOOLCHAIN_MOUNT` (`/opt/elcore50-toolchain-sdr-sdk`).

`CMAKE_INSTALL_LIBDIR` **обязан быть относительным**: иначе экспортированный
`_IMPORT_PREFIX` уедет в абсолютный путь и цель не найдётся в SDK. Ровно этим болел
`cxx17to14compatibility` — его собственный пресет задаёт libdir абсолютным.

---

## 4. Форк с bundled-зависимостью, конфликтующий с дистрибутивным пакетом

Случай `elvees-spdlog-devel`: дистрибутивный `libspdlog-devel` в Alt собран против внешнего
fmt v12, который требует явных специализаций `formatter` (`type_is_unformattable_for`), и агент
им не собирается. Пакуем форк ELVEES со встроенным fmt:

```spec
Name:      elvees-spdlog-devel
Version:   1.9.2
...
Conflicts: libspdlog-devel

%build
%cmake \
    -DSPDLOG_BUILD_SHARED=OFF \
    -DSPDLOG_FMT_EXTERNAL=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_BUILD_TYPE=Release
%cmake_build
```

---

## 5. noarch-обёртка над java-инструментом

Случай `fastddsgen` (репозиторий `dis/alt-infrastructure/fastddsgen`): в репозитории лежат
готовый jar и пусковой скрипт, spec их только раскладывает.

```spec
Name:      fastddsgen
Version:   ...
BuildArch: noarch
Requires:  java-17-openjdk-headless

%files
%_bindir/fastddsgen
%_datadir/fastddsgen/java/fastddsgen.jar
```

Раскладка репозитория: `bin/fastddsgen`, `share/fastddsgen/java/fastddsgen.jar`, `README.md`.

`fastddsgen` требует Java 11+ (`String.isBlank()`); на supermarket-wm по умолчанию Java 8 —
симптом `NoSuchMethodError: java.lang.String.isBlank()`, лечится `JAVA_HOME` на java-17.

---

## Чек-лист перед коммитом spec

- [ ] имя файла = `%name`
- [ ] `%setup -n %name`
- [ ] `%{?optflags_lto:...-ffat-lto-objects}`, если есть `.a`
- [ ] install-пути относительные, экспортированный CMake-конфиг без `/usr/lib64` и без `/home/`
- [ ] все транзитивные зависимости CMake-конфига перечислены в `Requires`
- [ ] для DSP — три skiplist'а
- [ ] пакет добавлен в `config/packages/alt.pkgs` **в порядке зависимостей**
- [ ] runtime-часть добавлена в `pkg.in/lists/mcom03` (mkimage-profiles), devel — в `config/sdk/sdk.pkgs`
- [ ] в сообщении коммита нет упоминаний Claude
