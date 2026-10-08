---
name: x86-usrp-bench
description: "Управление общим стендом Jenkins `x86+usrp0` с USRP X310: безопасное резервирование и освобождение ресурса, получение hostname/user/password из Jenkins Properties, первичная установка SSH-ключа без сохранения пароля, вход через `supermarket-wm` и `zeld-lpor-nmotin`, проверка USRP, доставка исходников, запуск detached-сборок и тестов с явными маркерами результата. Использовать при просьбах поработать на `x86+usrp0`, запустить host/USRP-тесты modules, проверить X310 или запустить на этом стенде DMR-приложение."
disable-model-invocation: true
---

# Стенд x86 + USRP

Работать со стендом только через `zeld-lpor-nmotin`; с локальной машины заходить как
`nmotin@supermarket-wm`. Значения `hostname`, `user` и `password` каждый раз брать из Properties
ресурса Jenkins, потому что они могут измениться.

## Подготовить helper

Helper хранится в `scripts/x86_usrp0.py`. Установить или обновить его на рабочем хосте:

```bash
scp -q -o ProxyJump=supermarket-wm \
  ~/.codex/skills/x86-usrp-bench/scripts/x86_usrp0.py \
  zeld-lpor-nmotin:/tmp/x86_usrp0.py
ssh supermarket-wm 'ssh zeld-lpor-nmotin \
  "mkdir -p ~/.local/bin && install -m 700 /tmp/x86_usrp0.py ~/.local/bin/x86-usrp0-stand"'
```

Не копировать `~/jenkins.token` и не выводить Properties целиком.

## Рабочий цикл

1. Проверить ресурс:

   ```bash
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand status'
   ```

2. Зарезервировать и повторно проверить состояние:

   ```bash
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand reserve'
   ```

   Если ресурс занят или зарезервирован другим пользователем, не обращаться к стенду.

3. При первом доступе установить отдельный ключ:

   ```bash
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand install-key'
   ```

   Helper получает пароль непосредственно из Jenkins для `ssh-copy-id`, не пишет его на диск и
   подтверждает вход с `BatchMode=yes`. Дальше использовать только ключ
   `~/.ssh/id_ed25519_x86_usrp0`.

4. Проверить стенд и USRP:

   ```bash
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand probe'
   ```

5. Выполнять команды через ключ:

   ```bash
   ssh supermarket-wm 'ssh -t zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand ssh'
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand ssh -- hostname'
   ```

6. После завершения всей работы освободить ресурс и проверить состояние:

   ```bash
   ssh supermarket-wm 'ssh zeld-lpor-nmotin ~/.local/bin/x86-usrp0-stand unreserve'
   ```

Не освобождать ресурс, пока на стенде идут detached-задачи или пользователь собирается проверить
их перед следующим этапом.

## Запуск долгих тестов

- Зафиксировать точный commit SHA; не подменять текущий HEAD старым CI-артефактом.
- Работать в отдельном каталоге вроде `~/mr165-tests`, не затрагивая чужие checkout'ы.
- Для работы дольше нескольких минут запускать runner на самом стенде через
  `setsid nohup ... < /dev/null &`, писать PID и лог.
- В runner использовать `set -Eeuo pipefail`, временны́е метки стадий и два явных маркера:
  `ТЕСТЫ УПАЛИ rc=N` через `trap ERR` и `ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ rc=0` в конце.
- Считать тесты запущенными только после маркера непосредственно перед `ctest`, а завершёнными —
  только по итоговому маркеру и JUnit-файлу. Отсутствие процесса не означает успех.
- Перед приложением, передающим в эфир, ещё раз проверить резерв и конфигурацию частот/усиления.
- Если пользователь разделил работу на этапы, не запускать следующий этап без его команды.

## Проверенные свойства стенда

- Jenkins resource: `x86+usrp0`, label `hardware:x86`.
- На 15 августа 2026 Properties указывали `dev@el001408-pc`; считать это снимком, а не константой.
- USRP X310 находилась по адресу `192.168.10.3`, serial `34ZK90X`; сверять через `probe`.
- Интерфейс к USRP был настроен как `192.168.10.1/24`.
- На host установлен Docker; крупный build-образ занимает много места, перед загрузкой проверять
  `df -h / /home`.

## Секреты

- Токены читать только на `zeld-lpor-nmotin` в процессе запроса.
- Никогда не печатать `git remote -v`: в старых checkout URL может содержать токен.
- Не выводить password Property, Basic-auth header, URL с credentials или окружение процесса.
- Не сохранять пароль стенда в helper, shell history, лог, memory или публичный артефакт.
