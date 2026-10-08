#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Pre-submission checks for the monthly FPI package.
    /tmp/fpi_venv/bin/python verify.py
All checks should pass before handing the package to Ilya.
"""
import os, json, hashlib, subprocess, zipfile, re, glob

REPO = "/home/auser/src/elvees/er_platform/runtime"
OUT = "/home/auser/src/elvees/other/MR_report/output-2026-09"
MONTH = "2026-09"            # YYYY-MM the dates must fall in
AUTHOR_EMAIL = "ishurupov@elvees.com"
TASKS = ["RadioManager_architecture", "IPC_vs_DDS_middleware"]


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest()


def main():
    ok = True

    # 1 & 2: diffs byte-match git + sha256 == manifest
    for t in TASKS:
        mpath = os.path.join(OUT, t, "manifest.json")
        if not os.path.exists(mpath):
            ok = False; print("FAIL missing manifest.json for", t); continue
        man = json.load(open(mpath))
        for e in man:
            disk = open(os.path.join(OUT, t, e["file"]), "rb").read()
            fresh = subprocess.run(["git", "-C", REPO, "diff", "--no-ext-diff", "--no-textconv",
                                    "--binary", "--full-index", e["hash"] + "^", e["hash"], "--"],
                                   capture_output=True).stdout
            if disk != fresh:
                ok = False; print("FAIL byte-match", t, e["hash"][:8])
            if hashlib.sha256(disk).hexdigest() != e["sha256"]:
                ok = False; print("FAIL sha256", t, e["hash"][:8])
    print("[1-2] diff byte-match + sha256 checked")

    # 3 & 4: dates/author/merges + unique patch_id
    for t in TASKS:
        mpath = os.path.join(OUT, t, "manifest.json")
        if not os.path.exists(mpath):
            continue
        man = json.load(open(mpath))
        for e in man:
            if e["author_date"][:7] != MONTH or e["commit_date"][:7] != MONTH:
                ok = False; print("FAIL date", t, e["hash"][:8])
            if e["email"] != AUTHOR_EMAIL:
                ok = False; print("FAIL author", t, e["hash"][:8])
            if len(e["parents"].split()) > 1:
                ok = False; print("FAIL merge-commit", t, e["hash"][:8])
        pids = [e["patch_id"] for e in man]
        if len(set(pids)) != len(pids):
            ok = False; print("FAIL duplicate patch_id", t)
    print("[3-4] dates/author/merges + patch_id unique checked")

    # 5: docx CRC + parse
    for f in glob.glob(os.path.join(OUT, "*.docx")) + glob.glob(os.path.join(OUT, "*", "*.docx")):
        if zipfile.ZipFile(f).testzip() is not None:
            ok = False; print("FAIL docx CRC", f)
    print("[5] docx CRC checked")

    # 7: files inside task zips == files on disk
    for t in TASKS:
        zp = os.path.join(OUT, t + ".zip")
        if not os.path.exists(zp):
            ok = False; print("FAIL missing zip", t); continue
        z = zipfile.ZipFile(zp)
        for info in z.infolist():
            if info.is_dir():
                continue
            disk = os.path.join(OUT, info.filename)
            if not os.path.exists(disk) or hashlib.sha256(z.read(info.filename)).hexdigest() != sha(disk):
                ok = False; print("FAIL zip-content mismatch", info.filename)
    print("[7] zip contents == folder checked")

    # 8: no secrets leaked
    pat = re.compile(r"glpat-[A-Za-z0-9_-]{20}|oauth2:[A-Za-z0-9]|PRIVATE-TOKEN|X-Auth-Token")
    for root, _, files in os.walk(OUT):
        for fn in files:
            if fn.endswith((".txt", ".md", ".json", ".diff")):
                p = os.path.join(root, fn)
                try:
                    if pat.search(open(p, "r", errors="ignore").read()):
                        ok = False; print("FAIL secret pattern in", p)
                except Exception:
                    pass
    print("[8] secret scan checked")

    print("\nRESULT:", "ALL OK" if ok else "FAILURES ABOVE")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
