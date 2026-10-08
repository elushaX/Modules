#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""One archive per task: zip each package folder whole (root = folder name).

System `zip` is NOT available — use python zipfile.
Archive name MUST match what the report references.
    /tmp/fpi_venv/bin/python make_zips.py
"""
import os, zipfile

OUT = "/home/auser/src/elvees/other/MR_report/output-2026-09"
TASKS = ["RadioManager_architecture", "IPC_vs_DDS_middleware"]  # edit per month


def main():
    for d in TASKS:
        src = os.path.join(OUT, d)
        zp = os.path.join(OUT, d + ".zip")
        if os.path.exists(zp):
            os.remove(zp)
        with zipfile.ZipFile(zp, "w", zipfile.ZIP_DEFLATED) as z:
            for root, _, files in os.walk(src):
                for fn in sorted(files):
                    full = os.path.join(root, fn)
                    z.write(full, os.path.relpath(full, OUT))
        print("built", d + ".zip")


if __name__ == "__main__":
    main()
