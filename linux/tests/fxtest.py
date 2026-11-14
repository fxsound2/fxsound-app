#!/usr/bin/env python3
"""FxSound Linux end-to-end test harness (host side).

Usage: fxtest.py <label> <launch-command...>
Launches FxSound with an isolated XDG_CONFIG_HOME, drives it through the tray's
com.canonical.dbusmenu interface, measures audio at the processor outputs and
checks PipeWire graph state. Restores the user's default sink, mute state and
removes test sinks on exit. Prints one line per check: PASS/FAIL/BLOCKED.
"""
import atexit, json, math, os, re, shutil, signal, struct, subprocess, sys, tempfile, time, wave

LABEL, CMD = sys.argv[1], sys.argv[2:]
WORK = tempfile.mkdtemp(prefix=f"fxtest-{LABEL}-", dir=os.environ.get("FXTEST_TMP"))
RESULTS = []
procs = []
modules = []

def sh(*a, check=False, **kw):
    return subprocess.run(a, capture_output=True, text=True, check=check, **kw)

ORIG_SINK = sh("pactl", "get-default-sink").stdout.strip()
ORIG_MUTE = {}
for line in sh("pactl", "list", "short", "sinks").stdout.splitlines():
    name = line.split("\t")[1]
    ORIG_MUTE[name] = "yes" in sh("pactl", "get-sink-mute", name).stdout

def cleanup():
    for p in procs:
        if p.poll() is None:
            p.send_signal(signal.SIGINT)
    time.sleep(0.5)
    for p in procs:
        if p.poll() is None:
            p.kill()
    for m in modules:
        sh("pactl", "unload-module", m)
    sh("pactl", "set-default-sink", ORIG_SINK)
    for name, muted in ORIG_MUTE.items():
        sh("pactl", "set-sink-mute", name, "1" if muted else "0")
atexit.register(cleanup)
signal.signal(signal.SIGTERM, lambda *a: sys.exit(1))

def record(name, ok, detail=""):
    status = "PASS" if ok is True else "FAIL" if ok is False else ok
    RESULTS.append((name, status, detail))
    print(f"{status:7} {name}  {detail}", flush=True)

# ---- graph helpers ---------------------------------------------------------
def nodes():
    out = sh("pw-dump").stdout
    try:
        objs = json.loads(out)
    except json.JSONDecodeError:
        return {}
    res = {}
    for o in objs:
        if o.get("type") == "PipeWire:Interface:Node":
            props = o.get("info", {}).get("props", {})
            res[o["id"]] = props
    return res

def fx_nodes():
    return [p.get("node.name") for p in nodes().values() if str(p.get("node.name", "")).startswith("FxSound")]

def links_from(port_prefix):
    out = sh("pw-link", "-l").stdout.splitlines()
    res, cur = [], None
    for line in out:
        if not line.startswith(" "):
            cur = line.strip()
        elif cur and cur.startswith(port_prefix) and "|->" in line:
            res.append(line.split("|->")[1].strip())
    return res

def processor_target():
    t = links_from("FxSound Processor:output_FL")
    return t[0].rsplit(":", 1)[0] if t else None

def configured_default():
    out = sh("pw-metadata", "0", "default.configured.audio.sink").stdout
    m = re.search(r'"name":\s*"([^"]+)"', out)
    return m.group(1) if m else None

def wait(cond, timeout=8.0, step=0.1):
    end = time.time() + timeout
    while time.time() < end:
        v = cond()
        if v:
            return v
        time.sleep(step)
    return cond()

# ---- tray (dbusmenu) -------------------------------------------------------
def tray_name(pid):
    out = sh("busctl", "--user", "list").stdout.splitlines()
    for line in out:
        f = line.split()
        if len(f) > 4 and f[0].startswith("org.kde.StatusNotifierItem") and f[1] == str(pid):
            return f[4]
    return None

def menu_layout(bus):
    out = sh("gdbus", "call", "--session", "-d", bus, "-o", "/com/fxsound/MenuBar",
             "-m", "com.canonical.dbusmenu.GetLayout", "--", "0", "-1", "[]").stdout
    items = {}
    for m in re.finditer(r"\((\d+), \{([^}]*)\}", out):
        props = dict(re.findall(r"'([\w-]+)': <([^>]*)>", m.group(2)))
        items[int(m.group(1))] = {k: v.strip("'") for k, v in props.items()}
    return items

def click(bus, item_id):
    sh("gdbus", "call", "--session", "-d", bus, "-o", "/com/fxsound/MenuBar",
       "-m", "com.canonical.dbusmenu.Event", "--", str(item_id), "clicked", "<0>", "0")

def find_item(items, label_re):
    for i, p in items.items():
        if re.search(label_re, p.get("label", "")):
            return i
    return None

# ---- audio -----------------------------------------------------------------
RATE = 48000
def make_tone(path, freq=1000.0, amp=0.25, secs=2.0, rate=RATE, stereo_diff=False):
    w = wave.open(path, "wb"); w.setnchannels(2); w.setsampwidth(2); w.setframerate(rate)
    frames = bytearray()
    for i in range(int(rate * secs)):
        s = amp * math.sin(2 * math.pi * freq * i / rate)
        r = s if not stereo_diff else amp * math.sin(2 * math.pi * freq * 1.5 * i / rate)
        frames += struct.pack("<hh", int(s * 32767), int(r * 32767))
    w.writeframes(bytes(frames)); w.close()

def analyze(path):
    w = wave.open(path); n = w.getnframes()
    d = struct.unpack("<%dh" % (n * 2), w.readframes(n))
    L, R = d[0::2], d[1::2]
    act = [i for i, x in enumerate(L) if abs(x) > 50]
    if not act:
        return {"frames": n, "active": 0, "peak": max(map(abs, d)) if d else 0, "rmsL": 0, "rmsR": 0, "clipped": 0}
    a, b = act[0], act[-1]
    seg = range(a + (b - a) // 4, b - (b - a) // 4)          # steady middle half
    rms = lambda ch: math.sqrt(sum(ch[i] ** 2 for i in seg) / max(1, len(seg)))
    return {"frames": n, "active": len(act), "peak": max(map(abs, d)), "rmsL": rms(L), "rmsR": rms(R),
            "clipped": sum(1 for x in d if abs(x) >= 32767)}

def capture_processor(tone, secs=2.6, target="FxSound"):
    out = os.path.join(WORK, f"cap{int(time.time()*1000)}.wav")
    rec = subprocess.Popen(["pw-record", "-P", "{ node.name=fxcap node.autoconnect=false }",
                            "--rate", str(RATE), "--channels", "2", "--format", "s16", out],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    procs.append(rec)
    if not wait(lambda: "fxcap" in sh("pw-link", "-i").stdout, 5):
        rec.kill(); return None
    sh("pw-link", "FxSound Processor:output_FL", "fxcap:input_FL")
    sh("pw-link", "FxSound Processor:output_FR", "fxcap:input_FR")
    sh("timeout", str(secs + 3), "pw-play", "--target", target, tone)
    time.sleep(0.3)
    rec.send_signal(signal.SIGINT); rec.wait(5)
    r = analyze(out); r["path"] = out
    return r

def db(x, ref):
    return 20 * math.log10(max(x, 1e-9) / max(ref, 1e-9))

# ---- app lifecycle -----------------------------------------------------------
CFG = os.path.join(WORK, "config")
def launch():
    env = dict(os.environ, XDG_CONFIG_HOME=CFG)
    p = subprocess.Popen(CMD, env=env, stdout=open(os.path.join(WORK, "app.log"), "a"),
                         stderr=subprocess.STDOUT, start_new_session=True)
    procs.append(p)
    return p

def app_pid():
    for p in procs:
        p.poll()                                  # reap exited children (no zombies)
    pids = sh("pgrep", "-x", "fxsound").stdout.split() + sh("pgrep", "-x", "FxSound").stdout.split()
    live = []
    for pid in pids:
        try:
            if open(f"/proc/{pid}/stat").read().rsplit(")", 1)[1].split()[0] != "Z":
                live.append(int(pid))
        except OSError:
            pass
    return max(live) if live else None

def app_pids():
    app_pid()
    pids = sh("pgrep", "-x", "fxsound").stdout.split() + sh("pgrep", "-x", "FxSound").stdout.split()
    return [p for p in pids if os.path.exists(f"/proc/{p}") and open(f"/proc/{p}/stat").read().rsplit(")", 1)[1].split()[0] != "Z"]

def stop_app(how="term"):
    pid = app_pid()
    if not pid:
        return True
    if how == "term":
        os.kill(pid, signal.SIGTERM)
    return wait(lambda: app_pid() is None, 10)

def start_and_wait():
    launch()
    ok = wait(lambda: len(fx_nodes()) >= 2 and processor_target(), 12)
    pid = app_pid()
    bus = wait(lambda: tray_name(pid), 8) if pid else None
    return bool(ok), pid, bus

# ============================================================================
def main():
    if app_pid():
        print("another FxSound is running; stop it first"); sys.exit(2)
    if configured_default() in (None, "FxSound"):
        sh("pactl", "set-default-sink", ORIG_SINK); time.sleep(0.5)
    orig_conf = configured_default()
    for name in ORIG_MUTE:                       # silence real outputs during tests
        sh("pactl", "set-sink-mute", name, "1")
    tone = os.path.join(WORK, "tone.wav"); make_tone(tone)
    stereo = os.path.join(WORK, "stereo.wav"); make_tone(stereo, stereo_diff=True)
    ref = analyze(tone)                           # input level reference
    print(f"# {LABEL}: orig default={ORIG_SINK} configured={orig_conf} input rms={ref['rmsL']:.0f}", flush=True)

    # T1 startup ------------------------------------------------------------
    ok, pid, bus = start_and_wait()
    record("T1 startup: FxSound + Processor nodes up", ok, str(fx_nodes()))
    if not ok:
        return
    record("T1 startup: FxSound is the configured default", configured_default() == "FxSound", str(configured_default()))
    record("T1 startup: processor feeds the user's default device", processor_target() == ORIG_SINK, str(processor_target()))
    record("T1 startup: tray menu reachable", bool(bus), str(bus))
    items = menu_layout(bus) if bus else {}

    # T2 processing on ------------------------------------------------------
    on = capture_processor(tone)
    record("T2 audio reaches processor output (power on)", bool(on and on["active"] > RATE),
           f"rms {on['rmsL']:.0f} ({db(on['rmsL'], ref['rmsL']):+.1f} dB) peak {on['peak']} clipped {on['clipped']}" if on else "no capture")
    record("T2 no clipping at -15 dBFS input", bool(on) and on["clipped"] == 0, "")

    # T3 power toggle -------------------------------------------------------
    off_id = find_item(items, r"^Turn Off$")
    if off_id:
        click(bus, off_id); time.sleep(0.8)
        off = capture_processor(tone)
        items = menu_layout(bus)
        record("T3 power off: output equals input level (bypass)",
               bool(off) and abs(db(off["rmsL"], ref["rmsL"])) < 0.5,
               f"{db(off['rmsL'], ref['rmsL']):+.2f} dB vs input" if off else "no capture")
        record("T3 power off: menu shows 'Turn On'", find_item(items, r"^Turn On$") is not None, "")
        click(bus, find_item(items, r"^Turn On$") or off_id); time.sleep(0.8)
        on2 = capture_processor(tone)
        # Level varies with the adaptive dynamic boost; require processing, not equality.
        record("T3 power on again: processing restored",
               bool(on2) and abs(db(on2["rmsL"], ref["rmsL"])) > 1.0,
               f"{db(on2['rmsL'], ref['rmsL']):+.2f} dB vs input" if on2 else "")
    else:
        record("T3 power toggle", "BLOCKED", "no 'Turn Off' item")

    # T4 presets ------------------------------------------------------------
    items = menu_layout(bus)
    presets = [(i, p["label"]) for i, p in sorted(items.items()) if 101 <= i < 200]
    levels = {}
    for i, name in presets[:6]:
        click(bus, i); time.sleep(0.8)
        cap = capture_processor(tone)
        it = menu_layout(bus)
        sel = it.get(i, {}).get("toggle-state") == "1"
        levels[name] = cap["rmsL"] if cap else None
        record(f"T4 preset '{name}': selected in menu and audio flows", sel and bool(cap and cap["active"] > RATE),
               f"{db(cap['rmsL'], ref['rmsL']):+.1f} dB clipped {cap['clipped']}" if cap else "no capture")
    distinct = len({round(v, -1) for v in levels.values() if v})
    record("T4 presets change the sound (distinct output levels)", distinct >= 2, f"{distinct} distinct of {len(levels)}")

    # T5 stereo channels -----------------------------------------------------
    st = capture_processor(stereo)
    if st:
        w = wave.open(st["path"])
        n = w.getnframes(); d = struct.unpack("<%dh" % (n * 2), w.readframes(n))
        zc = lambda ch: sum(1 for a, b in zip(ch, ch[1:]) if (a < 0) != (b < 0))
        zl, zr = zc(d[0::2]), zc(d[1::2])
        record("T5 stereo: left/right not swapped (R has higher pitch)", zr > zl * 1.2, f"zero crossings L={zl} R={zr}")

    # T6 output device selection + hot-plug ------------------------------------
    mod = sh("pactl", "load-module", "module-null-sink", "sink_name=fxtest_out",
             "sink_properties=device.description=FxTestOut").stdout.strip()
    modules.append(mod)
    item = wait(lambda: find_item(menu_layout(bus), r"FxTestOut"), 6)
    record("T6 hot-plug: new device appears in output menu", item is not None, "")
    if item:
        click(bus, item)
        tgt = wait(lambda: processor_target() == "fxtest_out", 5)
        record("T6 select output device: processor relinked to it", bool(tgt), str(processor_target()))
        sh("pactl", "unload-module", mod); modules.remove(mod)
        back = wait(lambda: processor_target() not in (None, "fxtest_out"), 6)
        record("T6 unplug selected device: falls back to another device", bool(back), str(processor_target()))
        record("T6 unplug: device removed from menu", wait(lambda: find_item(menu_layout(bus), r"FxTestOut") is None, 5), "")
        alive = capture_processor(tone)
        record("T6 audio still flows after unplug", bool(alive and alive["active"] > RATE), "")

    # T7 persistence ---------------------------------------------------------
    items = menu_layout(bus)
    if len(presets) >= 3:
        pick_id, pick_name = presets[2]
        click(bus, pick_id); time.sleep(1.0)
        stop_app()
        ok2, pid, bus = start_and_wait()
        sel = wait(lambda: menu_layout(bus).get(pick_id, {}).get("toggle-state") == "1", 6) if bus else False
        record(f"T7 preset '{pick_name}' persists across restart", bool(ok2 and sel), "")

    # T8 shutdown restores the user's default ---------------------------------
    stop_app()
    gone = wait(lambda: not fx_nodes(), 6)
    record("T8 SIGTERM: FxSound nodes removed", bool(gone), str(fx_nodes()))
    record("T8 SIGTERM: configured default restored by the app", configured_default() == orig_conf,
           f"{configured_default()} (expected {orig_conf})")

    ok3, pid, bus = start_and_wait()
    ex = find_item(menu_layout(bus), r"^Exit$") if bus else None
    if ex:
        click(bus, ex)
        record("T8 tray Exit: app quits", wait(lambda: app_pid() is None, 10) is True, "")
        record("T8 tray Exit: configured default restored", configured_default() == orig_conf, str(configured_default()))
    else:
        record("T8 tray Exit", "BLOCKED", "no Exit item"); stop_app()

    # T9 repeated start/stop --------------------------------------------------
    fails = 0
    for k in range(10):
        ok4, pid, _ = start_and_wait()
        if not ok4: fails += 1
        stop_app()
        if not wait(lambda: not fx_nodes(), 6): fails += 1
    record("T9 10x start/stop: all clean", fails == 0, f"{fails} failures")
    record("T9 default restored after loop", configured_default() == orig_conf, str(configured_default()))

    # T10 sustained playback ---------------------------------------------------
    ok5, pid, bus = start_and_wait()
    long_tone = os.path.join(WORK, "long.wav"); make_tone(long_tone, secs=60)
    t0 = os.times(); st0 = open(f"/proc/{pid}/stat").read().split()
    clk = os.sysconf("SC_CLK_TCK"); w0 = time.time()
    play = subprocess.Popen(["pw-play", "--target", "FxSound", long_tone]); procs.append(play)
    time.sleep(2)
    top = sh("pw-top", "-b", "-n", "3").stdout
    play.wait(70)
    st1 = open(f"/proc/{pid}/stat").read().split()
    cpu = (int(st1[13]) + int(st1[14]) - int(st0[13]) - int(st0[14])) / clk / (time.time() - w0) * 100
    rss = int(open(f"/proc/{pid}/statm").read().split()[1]) * os.sysconf("SC_PAGE_SIZE") / 2**20
    proc_lines = [l for l in top.splitlines() if "FxSound Processor" in l]
    record("T10 60 s playback: app still running", app_pid() == pid, "")
    record("T10 60 s playback: CPU usage", cpu < 15, f"{cpu:.1f}% of one core, RSS {rss:.0f} MiB")
    errs = [l.split()[8] if len(l.split()) > 8 else "?" for l in proc_lines]   # S ID QUANT RATE WAIT BUSY W/Q B/Q ERR
    record("T10 pw-top errors (xruns) on processor", "BLOCKED" if not proc_lines else all(e == "0" for e in errs),
           (proc_lines[-1].strip() if proc_lines else "processor not listed"))

    # T11 second instance -------------------------------------------------------
    p2 = launch(); time.sleep(5)
    pids = app_pids()
    record("T11 second launch: first instance keeps running", str(pid) in pids, f"running pids {pids}")
    record("T11 second launch: still exactly one FxSound sink", sum(1 for n in fx_nodes() if n == "FxSound") == 1, str(fx_nodes()))
    for p in pids:
        if p != str(pid): os.kill(int(p), signal.SIGTERM)
    time.sleep(3)
    record("T11 after second instance exits: first still routed", processor_target() is not None and configured_default() == "FxSound",
           f"target={processor_target()} default={configured_default()}")
    stop_app()

    # T12 PipeWire unavailable --------------------------------------------------
    env = dict(os.environ, XDG_CONFIG_HOME=CFG, PIPEWIRE_REMOTE="fxtest-no-such-socket")
    p3 = subprocess.Popen(CMD, env=env, stdout=open(os.path.join(WORK, "nopw.log"), "w"), stderr=subprocess.STDOUT, start_new_session=True)
    procs.append(p3); time.sleep(5)
    record("T12 no PipeWire server: app does not crash", p3.poll() is None or p3.returncode == 0,
           "running" if p3.poll() is None else f"exit {p3.returncode}")
    for extra in app_pids():                     # container launches are not our children
        os.kill(int(extra), signal.SIGTERM)
    wait(lambda: app_pid() is None, 10)
    if p3.poll() is None: os.killpg(p3.pid, signal.SIGTERM)

    print(f"# {LABEL}: {sum(r[1]=='PASS' for r in RESULTS)} PASS, {sum(r[1]=='FAIL' for r in RESULTS)} FAIL, "
          f"{sum(r[1]=='BLOCKED' for r in RESULTS)} BLOCKED; work dir {WORK}")
    json.dump(RESULTS, open(os.path.join(WORK, "results.json"), "w"))

if __name__ == "__main__":
    main()
