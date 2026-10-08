#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Объективные метрики по сотрудникам лаборатории: Jira + GitLab.

Запускать на хосте с доступом к корпоративной сети (supermarket-wm):
    scp lab_metrics.py supermarket-wm:~/
    ssh supermarket-wm 'python3 ~/lab_metrics.py --since 2026-01-01 --out ~/metrics.json'

Считает по каждому человеку:
  Jira   — закрыто задач за период, медиана цикла создание→resolution, открытых, из них старше 120 дней
  GitLab — коммиты (сопоставление по author_email), MR открытые/влитые, медиана дней до влития
  ревью  — комментарии к чужому коду, approve, merge (журнал событий)
"""
import argparse
import collections
import datetime
import json
import os
import ssl
import statistics
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

JIRA = "https://jira.elvees.com"
GITLAB = "https://git.elvees.com/api/v4"

# логин -> (ФИО, метка лаборатории, gitlab id)
PEOPLE = {
    "nmotin": ("Мотин Н.А.", "М", 659), "aavlin": ("Авлин А.В.", "М", 429),
    "kalyamovskiy": ("Алямовский К.Д.", "М", 1107), "ibaranov": ("Баранов И.А.", "М", 541),
    "gbasov": ("Басов Г.Т.", "М", 1369), "agoremykina": ("Горемыкина А.В.", "М", 1259),
    "kkizin": ("Кизин К.А.", "М", 1150), "akolmykov": ("Колмыков А.Ю.", "М", 1220),
    "ikorotkov": ("Коротков И.С.", "М", 1126), "anarizhnyy": ("Нарижный А.В.", "М", 1291),
    "dpetrakov": ("Петраков Д.С.", "М", 413), "vpleshakov": ("Плешаков В.А.", "М", 1038),
    "zstepanov": ("Степанов З.Ю.", "М", 966), "asunitskiy": ("Суницкий А.О.", "М", 1289),
    "akhazov": ("Хазов А.А.", "М", 553), "ishurupov": ("Шурупов И.В.", "М", 1174),
    "ichugunov": ("Чугунов И.В.", "Ч", 537), "lmoskvitin": ("Москвитин Л.В.", "Ч", 1087),
    "momelyukhin": ("Омелюхин М.И.", "Ч", 874), "eskidnov": ("Скиднов Е.А.", "Ч", 788),
    "vstrokov": ("Строков В.И.", "Ч", 1246), "vshapovalova": ("Шаповалова В.А.", "Ч", 1223),
}

PROJECTS = [
    "dis/sunrise/sunrise-platform/modules", "dis/sunrise/sunrise-platform/runtime",
    "dis/sunrise/sunrise-platform/infrastructure/jenkins-libs",
    "dis/sunrise/sunrise-platform/infrastructure/releaser",
    "dis/sunrise/sunrise-platform/radio-apps/runtime-examples",
    "dis/mcom03-drivers", "dis/code-style-convention", "dis/freertos-cpp", "dis/no-os",
    "dis/alt-infrastructure/build-scripts", "dis/alt-infrastructure/mkimage-profiles",
    "dis/alt-infrastructure/linux", "dis/alt-infrastructure/rpmsg",
    "dis/alt-infrastructure/elrpmsg", "dis/alt-infrastructure/virtio_rpmsg",
    "dis/alt-infrastructure/risc1", "dis/alt-infrastructure/esdr-rf-api",
    "dis/alt-infrastructure/elcore50", "dis/alt-infrastructure/elcorenn",
    "dis/alt-infrastructure/libgpiod", "dis/alt-infrastructure/pyfdt",
    "dis/nvcom05/linux-stable", "dis/nvcom05/buildroot", "dis/nvcom05/u-boot",
    "dis/nvcom05/delcore30m-tests",
    "dis/sbl/sbl", "dis/sbl/u-boot", "dis/sbl/ddrinit", "dis/sbl/arm-trusted-firmware",
    "dis/openwrt-infrastructure/rf_interface",
    "dis/openwrt-infrastructure/elv-freertos-unsecure-ipc",
    "dis/openwrt-infrastructure/packages/spectrumanalyzer",
    "dis/openwrt-infrastructure/arm-common-libs/alsa-interface",
    "dis/libs/mcom03_ipc", "dis/libs/polar_code", "dis/libs/dds/fast-dds",
    "dis/libs/dds/micro-xrce-dds-agent", "dis/libs/cxx17to14compatibility",
    "dis/sunrise/civil-radio-station/mo_template_dualcore",
    "dis/sunrise/civil-radio-station/dmr_modem_main_eliot",
    "dis/sunrise/civil-radio-station/mo_template_vocoder",
    "dis/sunrise/civil-radio-station/btstack",
    "dis/sunrise/civil-radio-station/ambe_vocoder_eliot",
    "dis/sunrise/sunrise-models/llc/mac-for-fast-ofdm-phy",
    "dis/sunrise/sunrise-models/rpi_lora_node/rpi4_sx1276_driver",
    "omo/netimit", "dis/qa-automation/bb-sentinel",
]

# репозитории с массовым импортом чужого кода — считаются, но помечаются
NOISY = {"dis/no-os", "dis/sunrise/civil-radio-station/btstack", "dis/nvcom05/linux-stable"}

CTX = ssl.create_default_context()
CTX.check_hostname = False
CTX.verify_mode = ssl.CERT_NONE


def token(name):
    return open(os.path.expanduser("~/%s.token" % name)).read().strip()


def fetch(url, headers, retries=3):
    for attempt in range(retries):
        try:
            return json.load(urllib.request.urlopen(
                urllib.request.Request(url, headers=headers), context=CTX, timeout=120))
        except urllib.error.HTTPError as e:
            return {"ERR": "HTTP %s" % e.code}
        except Exception:
            if attempt == retries - 1:
                raise
            time.sleep(2)


def days(a, b):
    return (datetime.date(*map(int, b[:10].split("-"))) -
            datetime.date(*map(int, a[:10].split("-")))).days


def jira_stats(logins, since, today):
    hdr = {"Authorization": "Bearer " + token("jira"), "Accept": "application/json"}
    out = {}
    for login in logins:
        jql = ("assignee = %s AND project in (SRISE, SRISE2, MCOM03SW, DEM, ALT) "
               "ORDER BY created ASC" % login)
        r = fetch(JIRA + "/rest/api/2/search?jql=" + urllib.parse.quote(jql) +
                  "&maxResults=300&fields=summary,status,created,resolutiondate", hdr)
        if "ERR" in r:
            out[login] = dict(closed=0, cycle=None, cycle_max=None, open=0, stale=0)
            continue
        cycles, closed, opened, stale = [], 0, 0, 0
        for issue in r["issues"]:
            f = issue["fields"]
            done = f.get("resolutiondate")
            if done:
                if done[:10] >= since:
                    closed += 1
                    cycles.append(days(f["created"], done))
            elif f["status"]["name"] not in ("Отменено", "Бэклог", "Готово", "Закрыто"):
                opened += 1
                if days(f["created"], today) > 120:
                    stale += 1
        out[login] = dict(closed=closed,
                          cycle=round(statistics.median(cycles)) if cycles else None,
                          cycle_max=max(cycles) if cycles else None,
                          open=opened, stale=stale)
    return out


def gitlab_repo_stats(logins, projects, since):
    hdr = {"PRIVATE-TOKEN": token("git")}
    commits = collections.defaultdict(collections.Counter)
    mr_open = collections.Counter()
    mr_merged = collections.Counter()
    ttm = collections.defaultdict(list)
    for path in projects:
        pid = urllib.parse.quote(path, safe="")
        page = 1
        while True:
            r = fetch(GITLAB + "/projects/%s/repository/commits?since=%sT00:00:00Z"
                      "&per_page=100&page=%d&all=true" % (pid, since, page), hdr)
            if isinstance(r, dict):
                break
            for c in r:
                login = (c.get("author_email") or "").lower().split("@")[0]
                if login in logins:
                    commits[login][path] += 1
            if len(r) < 100 or page > 60:
                break
            page += 1
        page = 1
        while True:
            r = fetch(GITLAB + "/projects/%s/merge_requests?created_after=%sT00:00:00Z"
                      "&per_page=100&page=%d&scope=all" % (pid, since, page), hdr)
            if isinstance(r, dict):
                break
            for m in r:
                login = (m["author"].get("username") or "").lower()
                if login not in logins:
                    continue
                mr_open[login] += 1
                if m["state"] == "merged":
                    mr_merged[login] += 1
                    if m.get("merged_at"):
                        ttm[login].append(days(m["created_at"], m["merged_at"]))
            if len(r) < 100 or page > 60:
                break
            page += 1
        print("  обойдён", path, file=sys.stderr)
    return commits, mr_open, mr_merged, ttm


def gitlab_events(people, since):
    hdr = {"PRIVATE-TOKEN": token("git")}
    out = {}
    for login, (_, _, uid) in people.items():
        evs, page = [], 1
        while True:
            r = fetch(GITLAB + "/users/%d/events?after=%s&per_page=100&page=%d"
                      % (uid, since, page), hdr)
            if isinstance(r, dict):
                break
            evs += r
            if len(r) < 100 or page > 15:
                break
            page += 1
        a = collections.Counter(e.get("action_name", "?") for e in evs)
        out[login] = dict(reviews=a.get("commented on", 0), approved=a.get("approved", 0),
                          merged=a.get("accepted", 0),
                          pushed=a.get("pushed to", 0) + a.get("pushed new", 0))
    return out


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--since", default="2026-01-01", help="дата начала периода, YYYY-MM-DD")
    p.add_argument("--today", default=datetime.date.today().isoformat())
    p.add_argument("--people", help="json {логин: [ФИО, лаборатория, gitlab_id]}")
    p.add_argument("--projects", help="файл со списком путей репозиториев, по одному в строке")
    p.add_argument("--out", default=os.path.expanduser("~/metrics.json"))
    args = p.parse_args()

    people = json.load(open(args.people)) if args.people else PEOPLE
    people = {k: tuple(v) for k, v in people.items()}
    projects = ([l.strip() for l in open(args.projects) if l.strip()]
                if args.projects else PROJECTS)

    print("Jira…", file=sys.stderr)
    jira = jira_stats(people, args.since, args.today)
    print("GitLab: репозитории…", file=sys.stderr)
    commits, mr_open, mr_merged, ttm = gitlab_repo_stats(set(people), projects, args.since)
    print("GitLab: события…", file=sys.stderr)
    events = gitlab_events(people, args.since)

    result = {}
    for login, (fio, lab, _) in people.items():
        by_proj = commits.get(login, collections.Counter())
        result[login] = dict(
            fio=fio, lab=lab, jira=jira[login], **events[login],
            commits=sum(by_proj.values()),
            commits_noisy=sum(v for pth, v in by_proj.items() if pth in NOISY),
            commits_by_project=dict(by_proj.most_common()),
            mr_open=mr_open.get(login, 0), mr_merged=mr_merged.get(login, 0),
            mr_days=round(statistics.median(ttm[login])) if ttm.get(login) else None)
    json.dump(result, open(args.out, "w"), ensure_ascii=False, indent=1)

    row = "%-16s %-3s %6s %6s %6s %6s %7s %7s %6s %6s %6s %6s %6s"
    print(row % ("ФИО", "лаб", "закр", "цикл", "макс", "стар", "комм", "(шум)",
                 "MR", "влито", "дней", "ревью", "appr"))
    for login, d in sorted(result.items(), key=lambda x: (x[1]["lab"], x[1]["fio"])):
        j = d["jira"]
        print(row % (d["fio"], d["lab"], j["closed"], j["cycle"], j["cycle_max"], j["stale"],
                     d["commits"], d["commits_noisy"], d["mr_open"], d["mr_merged"],
                     d["mr_days"], d["reviews"], d["approved"]))
    print("\nПодробности сохранены в", args.out, file=sys.stderr)


if __name__ == "__main__":
    main()
