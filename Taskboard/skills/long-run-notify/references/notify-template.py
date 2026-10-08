#!/usr/bin/env python3
"""Шаблон сторожа: ждёт маркер завершения на сборочном хосте и шлёт письмо.

Ставится на supermarket-wm и живёт независимо от сессии, из которой запущен:
    scp notify.py supermarket-wm:~/task-notify.py
    ssh supermarket-wm 'setsid nohup python3 ~/task-notify.py > ~/task-notify.log 2>&1 < /dev/null &'
    ssh supermarket-wm "pgrep -af 'task-notify[.]py'"   # ровно один экземпляр

Пароль владелец кладёт сам в ~/.buildalt-smtp (umask 077); скрипт его никуда
не пишет и не передаёт в командной строке.
"""
import os
import smtplib
import subprocess
import sys
import time
from datetime import datetime
from email.message import EmailMessage

HOST = "zeld-lpor-nmotin"
SMTP_HOST, SMTP_PORT = "owa.elvees.com", 587
SENDER = "nmotin@elvees.com"
RECIPIENTS = ["nmotin@elvees.com"]      # посторонних адресов не добавлять

PASSFILE = os.path.expanduser("~/.buildalt-smtp")
STATEFILE = os.path.expanduser("~/task-notify.state")
READYFILE = os.path.expanduser("~/task-READY.txt")

POLL_SECONDS = 120
GIVE_UP_HOURS = 12
UNREACHABLE_LIMIT = 30                  # ~1 час подряд недоступности

LOG = "/tmp/task-logs/chain-run.log"
DONE_MARK = "ВСЁ ГОТОВО rc=0"
FAIL_MARK = "ЗАДАЧА УПАЛА"
ARTIFACTS = ["~/buildalt/buildalt_tmp/build/images/vm/regular-xfce-skif/out",
             "~/buildalt/buildalt_tmp/build/sdk"]


def log(msg):
    line = f"{datetime.now():%Y-%m-%d %H:%M:%S}  {msg}"
    print(line, flush=True)
    with open(STATEFILE, "a") as f:
        f.write(line + "\n")


def remote(cmd, timeout=120):
    """Команда на рабочем хосте. Возвращает (rc, вывод); rc=255 - хост недоступен."""
    try:
        p = subprocess.run(
            ["ssh", "-o", "ConnectTimeout=20", "-o", "ServerAliveInterval=30",
             "-o", "BatchMode=yes", HOST, cmd],
            capture_output=True, text=True, timeout=timeout)
        return p.returncode, (p.stdout + p.stderr).strip()
    except Exception as e:
        return 255, f"{type(e).__name__}: {e}"


def outcome():
    """Исход по маркеру в логе, а не по наличию процесса.

    Имя процесса ненадёжно: шаблон pgrep совпадает и с собственной командной
    строкой, и с любым посторонним наблюдателем, у которого это имя в аргументах.
    Возвращает "done" / "failed" / None (ещё работает) / False (хост недоступен).
    """
    # tr - в логе сборки прогресс-бары разделены \r; grep -a - лог не всегда текстовый.
    rc, out = remote(f"tr '\\r' '\\n' < {LOG} 2>/dev/null | grep -a -e '{DONE_MARK}' "
                     f"-e '{FAIL_MARK}' | tail -1")
    if rc == 255:
        return False
    if DONE_MARK in out:
        return "done"
    if FAIL_MARK in out:
        return "failed"
    return None


def collect_report():
    parts = [f"echo '### хвост лога'; tr '\\r' '\\n' < {LOG} | tail -20"]
    for path in ARTIFACTS:
        parts.append(f"echo; echo '### {path}'; ls -la {path}/ 2>&1 | tail -12")
    _, out = remote("; ".join(parts), timeout=180)
    return out


def send(subject, body):
    if not os.path.exists(PASSFILE):
        log(f"нет файла с паролем {PASSFILE} - письмо не отправлено")
        return False
    with open(PASSFILE) as f:
        password = f.readline().strip()
    if not password:
        log(f"файл {PASSFILE} пуст - письмо не отправлено")
        return False

    ok = False
    for rcpt in RECIPIENTS:
        m = EmailMessage()
        m["From"], m["To"], m["Subject"] = SENDER, rcpt, subject
        m.set_content(body)
        try:
            with smtplib.SMTP(SMTP_HOST, SMTP_PORT, timeout=30) as s:
                s.starttls()
                s.login(SENDER, password)   # логин - полный адрес
                s.send_message(m)
            log(f"письмо отправлено: {rcpt}")
            ok = True
        except smtplib.SMTPAuthenticationError as e:
            # Форматы логина вслепую не перебирать: AD блокирует учётную запись.
            log(f"аутентификация не прошла ({e.smtp_code}) - нужен верный пароль")
        except Exception as e:
            log(f"не отправилось на {rcpt}: {type(e).__name__} {str(e)[:200]}")
    return ok


def finish(subject, body):
    """Итог сначала на диск, потом в почту: почта может отказать."""
    with open(READYFILE, "w") as f:
        f.write(subject + "\n\n" + body)
    log(f"итог записан в {READYFILE}")
    send(subject, body)


def main():
    log("сторож запущен, жду маркер завершения")
    deadline = time.time() + GIVE_UP_HOURS * 3600
    unreachable = 0

    while True:
        state = outcome()

        if state is False:
            unreachable += 1
            log(f"{HOST} недоступен ({unreachable})")
            # Недоступность хоста - это "не знаю", а не "готово".
            if unreachable >= UNREACHABLE_LIMIT:
                finish("watchdog: хост недоступен, состояние неизвестно",
                       f"{HOST} не отвечает больше часа. Состояние задачи неизвестно.\n"
                       f"Лог задачи: {LOG}\n")
                return 1
        elif state in ("done", "failed"):
            break
        else:
            unreachable = 0

        if time.time() > deadline:
            finish(f"watchdog: не дождался за {GIVE_UP_HOURS} ч",
                   f"Маркера завершения в {LOG} на {HOST} так и нет.\n"
                   f"Задача либо ещё идёт, либо повисла.\n")
            return 1

        time.sleep(POLL_SECONDS)

    log(f"задача завершилась ({state}), собираю отчёт")
    good = state == "done"
    subject = "watchdog: задача готова" if good else "watchdog: задача завершилась С ОШИБКОЙ"
    finish(subject,
           f"Хост: {HOST}\n"
           f"Время: {datetime.now():%Y-%m-%d %H:%M:%S}\n\n"
           f"{'Работа завершена успешно.' if good else 'В логе маркер ошибки.'}\n\n"
           f"Лог: {LOG}\n\n"
           f"{collect_report()}\n")
    log("сторож закончил работу")
    return 0 if good else 1


if __name__ == "__main__":
    sys.exit(main())
