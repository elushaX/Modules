#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Build per-task evidence packages (diffs, log, commits.txt, README, manifest).

Edit the CONFIG block for the target month, then run with the fpi venv python:
    /tmp/fpi_venv/bin/python gen_packages.py

Writes packages into OUT and dumps /tmp/fpi_meta.json for the DOCX step.
"""
import os, subprocess, json, hashlib

# ======================= CONFIG (edit each month) =======================
REPO = "/home/auser/src/elvees/er_platform/runtime"
OUT = "/home/auser/src/elvees/other/MR_report/output-2026-09"
REPO_URL = "https://git.elvees.com/dis/sunrise/sunrise-platform/runtime"
PERIOD = "01.09.2026–30.09.2026"

PACKAGES = {
    # folder name -> package config
    "RadioManager_architecture": {
        "theme": "Восход",
        "readme_title": "Развитие архитектуры управления вычислительными ядрами и радиоприложениями — сентябрь 2026",
        "intro": "Переработана архитектура runtime: CoreManager исполняет App Design, "
                 "RadioManager компилирует граф в поединичные инструкции и рассылает их на выполнение.",
        "platforms": "По конфигурации сборки общий API управления ядрами и графом используется для "
                     "x86/Linux, СКИФ (Arm/Elcore50) и Навиком (Nvcom05). Платформы определены по "
                     "конфигурации сборки, не по протоколам испытаний.",
        "commits": [
            ("7549d506", "implementation", "Объединение состояния выполнения графа в единый RuntimeContext."),
            # (sha_prefix, kind, description) ...
        ],
    },
}
# =======================================================================


def git(*args):
    return subprocess.run(["git", "-C", REPO, *args], capture_output=True, text=True).stdout

def git_bytes(*args):
    return subprocess.run(["git", "-C", REPO, *args], capture_output=True).stdout

def commit_meta(h):
    fmt = "%H%n%P%n%an%n%ae%n%aI%n%cI%n%s"
    o = git("show", "--no-patch", f"--format={fmt}", h).strip().split("\n")
    stat = git("show", "--stat", "--format=", h).strip().split("\n")
    statline = stat[-1].strip() if stat and stat[-1].strip() else ""
    return dict(full=o[0], short=o[0][:8], parents=o[1], an=o[2], ae=o[3],
                ad=o[4], cd=o[5], subj=o[6], statline=statline)

def get_diff(h):
    return git_bytes("diff", "--no-ext-diff", "--no-textconv", "--binary",
                     "--full-index", f"{h}^", h, "--")


def build(pkgdir, cfg):
    base = os.path.join(OUT, pkgdir)
    cdir = os.path.join(base, "commits")
    os.makedirs(cdir, exist_ok=True)
    metas = []
    for h, kind, desc in cfg["commits"]:
        m = commit_meta(h); m["kind"] = kind; m["desc"] = desc
        diff = get_diff(h)
        with open(os.path.join(cdir, f"{m['full']}.diff"), "wb") as f:
            f.write(diff)
        m["sha256"] = hashlib.sha256(diff).hexdigest()
        m["patch_id"] = subprocess.run(["git", "-C", REPO, "patch-id", "--stable"],
                                       input=diff, capture_output=True).stdout.decode().split()[0] if diff else ""
        m["difffile"] = f"commits/{m['full']}.diff"
        metas.append(m)
    with open(os.path.join(base, "commits.txt"), "w") as f:
        for m in metas: f.write(f"{m['full']} {m['subj']}\n")
    with open(os.path.join(base, "log.txt"), "w") as f:
        f.write("\n\n".join(git("show", "--no-patch", "--format=fuller", m["full"]).rstrip() for m in metas) + "\n")
    with open(os.path.join(base, "README.txt"), "w") as f:
        f.write(cfg["readme_title"] + "\n\n\n")
        f.write(f"Репозиторий: {REPO_URL}\n\n")
        for m in metas:
            f.write(f"{m['full']}\n{m['desc']}\n{REPO_URL}/-/commit/{m['full']}\n\n")
    manifest = [{
        "hash": m["full"], "parents": m["parents"], "author": m["an"], "email": m["ae"],
        "author_date": m["ad"], "commit_date": m["cd"], "subject": m["subj"],
        "url": f"{REPO_URL}/-/commit/{m['full']}", "description": m["desc"], "kind": m["kind"],
        "file": m["difffile"], "sha256": m["sha256"], "patch_id": m["patch_id"],
    } for m in metas]
    with open(os.path.join(base, "manifest.json"), "w") as f:
        json.dump(manifest, f, ensure_ascii=False, indent=2)
    return base, metas


def main():
    os.makedirs(OUT, exist_ok=True)
    dump = {}
    for pkgdir, cfg in PACKAGES.items():
        base, metas = build(pkgdir, cfg)
        print(f"built {pkgdir}: {len(metas)} commits")
        dump[pkgdir] = {"base": base, "cfg": cfg, "metas": [
            {k: m[k] for k in ("full", "short", "subj", "an", "ad", "cd", "desc", "statline", "kind", "difffile", "sha256")}
            for m in metas]}
    dump["_period"] = PERIOD
    with open("/tmp/fpi_meta.json", "w") as f:
        json.dump(dump, f, ensure_ascii=False, indent=2)
    print("meta -> /tmp/fpi_meta.json")


if __name__ == "__main__":
    main()
