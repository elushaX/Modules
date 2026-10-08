import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.*;
public class DumpEngine extends GhidraScript {
  public void run() throws Exception {
    long[][] ranges = { {0x1400c5000L,0x1400e6000L}, {0x1400e8000L,0x140118000L}, {0x140136000L,0x140143000L}, {0x14005f000L,0x140073000L}, {0x14011e000L,0x14012e000L} };
    DecompInterface d = new DecompInterface(); d.openProgram(currentProgram);
    String[] args = getScriptArgs();
    File out = new File(args.length > 0 ? args[0] : "/tmp/leo-decompiled");
    out.mkdirs();
    for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
      long a = f.getEntryPoint().getOffset(); boolean in=false;
      for (long[] r: ranges) if (a>=r[0] && a<r[1]) in=true;
      if (!in) continue;
      DecompileResults res = d.decompileFunction(f, 60, monitor);
      if (res==null || res.getDecompiledFunction()==null) continue;
      try (PrintWriter w = new PrintWriter(new File(out, String.format("%x.c", a)))) { w.print(res.getDecompiledFunction().getC()); }
    }
  }
}
