# Leonardo reference tooling

Scripts used while porting Leonardo features into Sketch2D and publishing builds.

| File | Purpose |
| --- | --- |
| `publish-sketch2d.sh` | Build the release APK, publish it with `latest.json` to ilusha-basic.online/sketch2d |
| `publish-leo.sh`, `publish-stroke2d.sh` | Same for the leo and stroke2d channels |
| `release-history.py` | Keeps the last 60 releases in `~/.<app>-release/history.json` and writes them into `latest.json` as `releases`, so the in-app updater can list every missed build |
| `send-apk.sh` | Upload the APK to the Matrix room, then publish |
| `ghidra/DumpEngine.java` | Ghidra headless post script that decompiles the engine address ranges of `leonardo.0.17.70.win64.exe` into one C file per function |

Decompile the reference engine:

```
/opt/ghidra/support/analyzeHeadless /tmp/leo-ghidra leo -import leonardo.0.17.70.win64.exe \
	-scriptPath leo/ghidra -postScript DumpEngine.java /tmp/leo-decompiled -deleteProject
```

The analysis built from it lives in `Sketch2D/docs/research/LeonardoEngineAnalysis.md`.
