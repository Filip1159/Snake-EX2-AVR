#!/usr/bin/env python3
"""build_oracle.py — compile the ORIGINAL decompiled engine (class b) headless and record traces.

The decompiled sources live in reference/ (copyrighted, git-ignored). They are copied to
build/oracle/, minimally patched so they compile against the MIDP stubs in tools/oracle/stubs,
and driven by tools/oracle/src/Oracle.java. Output: build/oracle/traces/*.csv (one line per tick).

Needs a JDK (javac). Set JAVA_HOME or JAVAC, or put javac on PATH.
Usage: python3 tools/oracle/build_oracle.py [--traces]
CSV columns: tick,keys,len,Q,r,j,H,Ja,Da,ea,fa,ga,Ga,Fa,b,c,C,Aa,ma,J,segments(hex),events
"""
import itertools, os, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
OUT = ROOT / "build" / "oracle"
SRC = ROOT / "reference" / "decompiled" / "com" / "nokia" / "mid" / "appl" / "snake"


def find(tool):
    for env in ("JAVA_HOME",):
        if os.environ.get(env):
            p = Path(os.environ[env]) / "bin" / tool
            if p.exists():
                return str(p)
    for p in [shutil.which(tool)] + [str(x) for x in Path("/tmp/opencode").glob(f"jdk*/bin/{tool}")]:
        if p:
            return p
    sys.exit(f"{tool} not found (set JAVA_HOME)")


def patch(text):
    def rep(a, b):
        nonlocal text
        assert a in text, a[:60]
        text = text.replace(a, b)
    rep("((b)object)", "this")
    i, j = text.index("    static void a(Snake object) {"), text.index("    public b(Snake snake, boolean bl) {")
    text = text[:i] + "    static void a(Snake object) { D = new byte[6][378]; }\n\n" + text[j:]
    rep("public b(Snake snake, boolean n, int n2) {", "public b(Snake snake, boolean nb, int n2) {\n        int n;")
    rep("            if (n != 0) {\n                try {\n                    RecordStore.deleteRecordStore", "            if (nb) {\n                try {\n                    RecordStore.deleteRecordStore")
    rep("directGraphics.getPixels(e, 1, 24", "directGraphics.getPixels(new int[1], 1, 24")
    for a in ("SnakeCanvas.a(graphics, 6, 114, this.H, 6);", "SnakeCanvas.a(graphics, 69, 114, this.Ea, 4);", "SnakeCanvas.a(graphics, 69, 114, this.fa, 4);"):
        rep(a, "")
    rep("Object object = byteArrayOutputStream.toByteArray();\n            recordStore.addRecord((byte[])object, 0, ((Object)object).length);",
        "byte[] object = byteArrayOutputStream.toByteArray();\n            recordStore.addRecord(object, 0, object.length);")
    return text


def build():
    javac, pkg = find("javac"), OUT / "src" / "com" / "nokia" / "mid" / "appl" / "snake"
    shutil.rmtree(OUT / "src", ignore_errors=True)
    shutil.copytree(ROOT / "tools" / "oracle" / "stubs", OUT / "src")
    shutil.copy(ROOT / "tools" / "oracle" / "src" / "Oracle.java", pkg / "Oracle.java")
    for f in ("GlobalDefs.java", "ResourceDefs.java"):
        shutil.copy(SRC / f, pkg / f)
    (pkg / "b.java").write_text(patch((SRC / "b.java").read_text()))
    (OUT / "classes").mkdir(parents=True, exist_ok=True)
    subprocess.check_call([javac, "-nowarn", "-d", str(OUT / "classes")] + [str(p) for p in (OUT / "src").rglob("*.java")])


def traces():
    java = find("java")
    d = ROOT / "reference" / "d"
    tdir = OUT / "traces"
    shutil.rmtree(tdir, ignore_errors=True)
    tdir.mkdir(parents=True)
    n = 0
    for maze, speed, seed in itertools.product((-1, 0, 1, 2, 3, 4), (0, 2, 5), (11, 12, 13)):
        for prob in (0.06, 0.25, -1, -2):
            name = f"classic_m{maze}_s{speed}_r{seed}_p{int(prob*100)}.csv"
            with open(tdir / name, "w") as f:
                subprocess.check_call([java, "-cp", str(OUT / "classes"), "com.nokia.mid.appl.snake.Oracle", str(d),
                                       "0", str(maze), str(speed), str(seed), str(seed + 100), "6000", str(prob)], stdout=f)
            n += 1
    print(f"{n} traces in {tdir}")


def classic_all_traces():
    """G9: one survival-bot trace for every Classic maze x speed combination.
    The main --traces set only samples speeds 0, 2 and 5; this set covers 1, 3 and 4
    as well, so the cadence/score path is oracle-checked for all 36 combinations."""
    java = find("java")
    d = ROOT / "reference" / "d"
    tdir = OUT / "traces_classic_all"
    shutil.rmtree(tdir, ignore_errors=True)
    tdir.mkdir(parents=True)
    n = 0
    for maze in (-1, 0, 1, 2, 3, 4):
        for speed in range(6):
            name = f"classic_m{maze}_s{speed}_r77.csv"
            with open(tdir / name, "w") as f:
                subprocess.check_call([java, "-cp", str(OUT / "classes"), "com.nokia.mid.appl.snake.Oracle", str(d),
                                       "0", str(maze), str(speed), "77", "177", "6000", "-2"], stdout=f)
            n += 1
    print(f"{n} all-combination Classic traces in {tdir}")


def pu_traces():
    """Extreme-mode sandbox traces with the power-up-seeking bot. The set is chosen to touch
    all ten power-ups and the slot/coin/stamp/lights state machines."""
    java = find("java")
    d = ROOT / "reference" / "d"
    tdir = OUT / "traces_pu"
    shutil.rmtree(tdir, ignore_errors=True)
    tdir.mkdir(parents=True)
    cases = [
        (0, 45), (2, 27), (4, 23), (4, 26), (4, 30), (4, 45),
        (5, 14), (5, 15), (5, 17), (5, 19), (5, 20), (5, 23),
        (5, 26), (5, 29), (5, 30), (5, 33), (5, 38), (5, 42),
    ]
    for speed, seed in cases:
        name = f"extreme_s{speed}_r{seed}.csv"
        with open(tdir / name, "w") as f:
            subprocess.check_call([java, "-cp", str(OUT / "classes"), "com.nokia.mid.appl.snake.Oracle", str(d),
                                   "2", "-1", str(speed), str(seed), str(seed + 100), "8000", "-3"], stdout=f)
    print(f"{len(cases)} power-up traces in {tdir}")


def campaign_traces():
    """Extreme campaign harness (Oracle mode 3): score-gated portal, teleport entry,
    shrink-to-2, maze cycling and level-start score. The final columns are x,z,Y,X,y,da."""
    java = find("java")
    d = ROOT / "reference" / "d"
    tdir = OUT / "traces_campaign"
    shutil.rmtree(tdir, ignore_errors=True)
    tdir.mkdir(parents=True)
    cases = [(s, r) for s in (0, 2, 5) for r in (11, 23, 42)]
    for speed, seed in cases:
        name = f"campaign_s{speed}_r{seed}.csv"
        with open(tdir / name, "w") as f:
            subprocess.check_call([java, "-cp", str(OUT / "classes"), "com.nokia.mid.appl.snake.Oracle", str(d),
                                   "3", "-1", str(speed), str(seed), str(seed + 100), "20000", "0"], stdout=f)
    print(f"{len(cases)} campaign traces in {tdir}")


def campaign_natural_traces():
    """Natural Extreme play with the power-up-seeking bot (Oracle mode 1, z=0): portals
    appear during normal movement/food play. The final columns are x,z,Y,X,y,da."""
    java = find("java")
    d = ROOT / "reference" / "d"
    tdir = OUT / "traces_campaign_natural"
    shutil.rmtree(tdir, ignore_errors=True)
    tdir.mkdir(parents=True)
    cases = [(5, r) for r in (14, 17, 19, 20, 23, 26, 29, 30, 33, 38, 42, 45)]
    for speed, seed in cases:
        name = f"campaign_s{speed}_r{seed}.csv"
        with open(tdir / name, "w") as f:
            subprocess.check_call([java, "-cp", str(OUT / "classes"), "com.nokia.mid.appl.snake.Oracle", str(d),
                                   "1", "-1", str(speed), str(seed), str(seed + 100), "15000", "-3"], stdout=f)
    print(f"{len(cases)} natural campaign traces in {tdir}")


if __name__ == "__main__":
    build()
    if "--traces" in sys.argv:
        traces()
    if "--classic-all" in sys.argv:
        classic_all_traces()
    if "--pu-traces" in sys.argv:
        pu_traces()
    if "--campaign-traces" in sys.argv:
        campaign_traces()
    if "--campaign-natural" in sys.argv:
        campaign_natural_traces()
