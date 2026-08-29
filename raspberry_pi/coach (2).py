#!/usr/bin/env python3
"""
coach.py  -  Vallamkali Sync Coach

ESP32 sends 3 strokes per rower (9 total).

Python  : collects the packets, averages each rower, works out how far each
          rower sits from the other two on timing, angle and power, and finds
          the single largest deviation.
Gemma   : decides whether that deviation is a real fault that needs a
          correction alert, or normal human variation. This is the call
          nothing in the Python code makes.

    python coach.py --test     check the model, 8 cases
    python coach.py --mock     run with fake strokes
    python coach.py            run on real UART
"""

import argparse
import json
import re
import sys
import time
from collections import deque

import requests

try:
    import serial
except ImportError:
    serial = None

URL = "http://127.0.0.1:11434/api/generate"
MODEL = "gemma3:4b"

SYSTEM = """You are a rowing coach watching three rowers in one boat.

You are told how far one rower has drifted from the other two on one measure.
Decide whether that rower needs a correction alert.

Scale: a full stroke takes about 100 centiseconds, a normal catch angle is
about 42 degrees, and normal stroke power is about 28 units. Rowers are never
identical, so a little drift is expected. A drift that is a large fraction of
these numbers means that rower is out of step with the crew.

Examples:
Rower 1 is 4 centiseconds behind the other two rowers on timing. -> N
Rower 2 is 35 centiseconds behind the other two rowers on timing. -> Y
Rower 3 has 18 degrees more catch angle than the other two rowers. -> Y
Rower 1 has 5 degrees more catch angle than the other two rowers. -> N
Rower 2 has 16 units more stroke power than the other two rowers. -> Y
Rower 3 has 4 units less stroke power than the other two rowers. -> N

Answer Y if this rower should be given a correction alert.
Answer N if this is normal and no alert is needed.

Answer with only Y or N."""

SHORT_SYSTEM = """You are a rowing coach. One rower has drifted from the other
two. Decide if they need a correction alert. A stroke is about 100
centiseconds, a catch angle about 42 degrees, stroke power about 28 units.

Rower 1 is 4 centiseconds behind the other two rowers on timing. -> N
Rower 2 is 35 centiseconds behind the other two rowers on timing. -> Y
Rower 3 has 18 degrees more catch angle than the other two rowers. -> Y
Rower 1 has 5 degrees more catch angle than the other two rowers. -> N
Rower 2 has 16 units more stroke power than the other two rowers. -> Y
Rower 3 has 4 units less stroke power than the other two rowers. -> N

Answer only Y or N."""

FAST_SYSTEM = """A rower has drifted from the rest of the crew.
Small drift is normal and needs no alert.
Answer N if the number is 10 or less.
Answer Y if the number is clearly more than 10.

timing 45 -> Y
timing 5 -> N
angle 18 -> Y
angle 6 -> N
power 16 -> Y
power 4 -> N

Answer only Y or N."""

MEASURES = {
    "T": ("TIMING", "timing", "centiseconds"),
    "A": ("ANGLE", "catch angle", "degrees"),
    "P": ("POWER", "stroke power", "units"),
}
LED = {"TIMING": "RED", "ANGLE": "YELLOW", "POWER": "BLUE"}

buf = {1: deque(maxlen=3), 2: deque(maxlen=3), 3: deque(maxlen=3)}
sess = requests.Session()
QUIET = False
SHORT = False
FAST = False


def say(*a):
    if not QUIET:
        print(*a)


# ---------------------------------------------------------------- gemma

def ask_gemma(question):
    """One yes/no judgement. Returns True, False, or None if the call failed."""
    body = {
        "model": MODEL,
        "system": FAST_SYSTEM if FAST else (SHORT_SYSTEM if SHORT else SYSTEM),
        "prompt": question,
        "stream": False, "keep_alive": -1,
        "format": {"type": "string", "enum": ["Y", "N"]},
        "options": {"temperature": 0, "top_k": 1, "num_predict": 3,
                    "num_ctx": 256 if FAST else 512, "seed": 0},
    }
    t = time.time()
    try:
        r = sess.post(URL, json=body, timeout=30)
        r.raise_for_status()
        data = r.json()
    except Exception as e:
        say("  gemma failed:", e)
        return None

    raw = data.get("response", "")
    m = re.search(r"[YN]", raw.strip().strip('"').upper())
    ans = None if not m else (m.group(0) == "Y")
    say(f"  gemma: {raw!r} -> {'ALERT' if ans else 'no alert'}   "
        f"{time.time()-t:.2f}s  ptok={data.get('prompt_eval_count', 0)}")
    return ans


# ---------------------------------------------------------------- packets

def add_stroke(pkt):
    try:
        r = int(pkt["rower"])
        sid = int(pkt["stroke_id"])
        t0 = int(pkt.get("t0", time.monotonic() * 1000))
    except (KeyError, TypeError, ValueError):
        return
    if r not in buf or any(s["sid"] == sid for s in buf[r]):
        return
    buf[r].append({"sid": sid, "t0": t0,
                   "angle": float(pkt.get("angle", 0)),
                   "acc": float(pkt.get("peak_acc", 0))})


def aligned_ids():
    """The 3 newest stroke_ids all rowers share, or None.

    Each Gemma call blocks for a second or two, so the buffers drift out of
    step. Without this you compare R1 stroke 6 against R3 stroke 4.
    """
    sets = [{s["sid"] for s in buf[r]} for r in (1, 2, 3)]
    common = sorted(set.intersection(*sets))
    return common[-3:] if len(common) >= 3 else None


def find_worst(ids):
    """Average each rower, compare against the other two, return the single
    largest deviation as (rower, measure_key, signed_value).

    This is arithmetic only. It does NOT decide whether the deviation matters.
    """
    at = {r: {s["sid"]: s for s in buf[r]} for r in (1, 2, 3)}
    starts = [min(at[r][sid]["t0"] for r in (1, 2, 3)) for sid in ids]

    avg = {}
    for r in (1, 2, 3):
        rows = [at[r][sid] for sid in ids]
        avg[r] = {
            "T": sum((s["t0"] - starts[i]) / 10 for i, s in enumerate(rows)) / 3,
            "A": sum(s["angle"] for s in rows) / 3,
            "P": sum(s["acc"] for s in rows) * 10 / 3,
        }

    worst = None
    for r in (1, 2, 3):
        others = [o for o in (1, 2, 3) if o != r]
        for k in MEASURES:
            d = round(avg[r][k] - sum(avg[o][k] for o in others) / 2)
            if worst is None or abs(d) > abs(worst[2]):
                worst = (r, k, d)
    return worst


def parse_batch(line):
    """Parse one compact batch line from the ESP32.

        D|t1,t2,t3,ang,pow|t1,t2,t3,ang,pow[|t1,t2,t3,ang,pow]

    Two or three rower groups. Returns {rower: {...}} or None if malformed.
    """
    parts = line.strip().split("|")
    if len(parts) < 3 or len(parts) > 4 or parts[0] != "D":
        return None
    out = {}
    for i, chunk in enumerate(parts[1:], start=1):
        f = chunk.split(",")
        if len(f) != 5:
            return None
        try:
            out[i] = {"T": [int(f[0]), int(f[1]), int(f[2])],
                      "A": float(f[3]), "P": float(f[4])}
        except ValueError:
            return None
    return out


def worst_from_batch(b):
    """Largest deviation in the batch, as (rower, measure_key, signed_value).

    Three rowers: each is compared against the average of the other two, so
    the odd one out stands apart.

    Two rowers: rower 1 is the stroke seat and sets the rhythm, so only
    rower 2 is judged, against rower 1. With only two rowers the numbers
    alone cannot say which of the pair is wrong, so the boat needs a
    reference. That is how a real crew works.

    Arithmetic only. It does not decide whether the deviation matters.
    """
    rowers = sorted(b)
    n = len(rowers)

    avg = {}
    for r in rowers:
        offs = []
        for i in range(3):
            first = min(b[x]["T"][i] for x in rowers)
            offs.append((b[r]["T"][i] - first) / 10.0)
        avg[r] = {"T": sum(offs) / 3, "A": b[r]["A"], "P": b[r]["P"]}

    if n == 2:
        ref, judged = rowers[0], rowers[1]
        worst = None
        for k in MEASURES:
            d = round(avg[judged][k] - avg[ref][k])
            if worst is None or abs(d) > abs(worst[2]):
                worst = (judged, k, d)
        return worst

    worst = None
    for r in rowers:
        others = [o for o in rowers if o != r]
        for k in MEASURES:
            d = round(avg[r][k] - sum(avg[o][k] for o in others) / len(others))
            if worst is None or abs(d) > abs(worst[2]):
                worst = (r, k, d)
    return worst


SHORT_LABEL = {"T": "timing", "A": "angle", "P": "power"}


def make_question(rower, key, value):
    """In fast mode the question matches the terse examples exactly, e.g.
    "timing 25". Matching formats is what makes short examples work.
    The rower number is left out on purpose: Python already knows who it is,
    Gemma only judges whether the size of the drift warrants an alert."""
    if FAST:
        return f"{SHORT_LABEL[key]} {abs(value)}"

    name, label, unit = MEASURES[key]
    direction = "ahead of" if value < 0 else "behind"
    if key == "T":
        return (f"Rower {rower} is {abs(value)} {unit} {direction} "
                f"the other two rowers on {label}.")
    more = "less" if value < 0 else "more"
    return (f"Rower {rower} has {abs(value)} {unit} {more} {label} "
            f"than the other two rowers.")


def make_decision(rower, key, value, alert):
    if not alert:
        return {"type": "DECISION", "target": 0, "status": "IN_SYNC",
                "error": "NONE", "led": "GREEN", "vibration": "OFF"}
    fault = MEASURES[key][0]
    err = "EARLY" if value < 0 else "LATE"
    return {"type": "DECISION", "target": rower, "status": "OUT_OF_SYNC",
            "error": err, "fault": fault,
            "led": LED[fault], "vibration": "ON"}


FAULT_CODE = {"T": 1, "A": 2, "P": 3}     # timing, angle, power

# A stroke is ~100 cs. A genuine sync fault is tens of cs. Anything past this
# is a data problem, not a rower.
MAX_PLAUSIBLE_CS = 200


def make_packet(rower, key, alert):
    """Two digits plus newline. That is the whole message.

        00   everyone fine, no alert
        31   rower 3, timing fault
        32   rower 3, angle fault
        33   rower 3, power fault

    Second digit 0 means no alert, so the Central can test it with one
    comparison and still know the reason.
    """
    return f"{rower}{FAULT_CODE[key]}" if alert else "00"


# ---------------------------------------------------------------- testing

TESTS = [
    ((1, "T", 2), False),
    ((2, "T", -3), False),
    ((3, "T", 38), True),
    ((1, "T", -41), True),
    ((3, "A", 22), True),
    ((2, "A", 3), False),
    ((3, "P", 26), True),
    ((1, "P", -2), False),
]


def run_tests():
    print("warming up...")
    ask_gemma(make_question(*TESTS[0][0]))
    print()
    passed = 0
    for (rower, key, value), want in TESTS:
        q = make_question(rower, key, value)
        print("  " + q)
        got = ask_gemma(q)
        ok = got == want
        passed += ok
        print(f"  {'PASS' if ok else 'FAIL'}  want={'ALERT' if want else 'no alert'}\n")
    print(f"{passed}/{len(TESTS)} correct")
    if passed < len(TESTS) - 1:
        print("Try:  --model gemma3:4b")


def fake_strokes():
    """R3 drifts late, then their angle opens up too."""
    import random
    sid = 0
    while True:
        sid += 1
        base = int(time.monotonic() * 1000)
        for r in (1, 2, 3):
            bad = r == 3
            yield {"type": "STROKE", "rower": r, "stroke_id": sid,
                   "t0": base + (min(400, 60 * sid) if bad else 0)
                         + random.randint(-20, 20),
                   "duration_ms": 1000,
                   "angle": 42 + (min(20, 3 * sid) if bad else 0)
                            + random.uniform(-1, 1),
                   "peak_acc": 2.8 + random.uniform(-0.1, 0.1)}
        time.sleep(1.0)


# ---------------------------------------------------------------- main

def main():
    global MODEL, QUIET, SHORT, FAST
    ap = argparse.ArgumentParser()
    ap.add_argument("--mock", action="store_true")
    ap.add_argument("--test", action="store_true")
    ap.add_argument("--quiet", action="store_true",
                    help="no terminal output, UART only")
    ap.add_argument("--short", action="store_true",
                    help="shorter system prompt, faster")
    ap.add_argument("--fast", action="store_true",
                    help="tiny prompt, fastest")
    ap.add_argument("--json", action="store_true",
                    help="send the full JSON packet instead of the 2-char one")
    ap.add_argument("--model", default=MODEL)
    ap.add_argument("--port", default="/dev/ttyAMA0")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()
    MODEL = args.model
    QUIET = args.quiet
    SHORT = args.short
    FAST = args.fast

    if args.test:
        run_tests()
        return

    ser = None
    if not args.mock:
        if serial is None:
            sys.exit("pip install pyserial, or use --mock")
        ser = serial.Serial(args.port, args.baud, timeout=1)
        say(f"serial open {args.port}")

    say("warming up gemma...")
    ask_gemma(make_question(1, "T", 2))
    say("ready\n")

    source = fake_strokes() if args.mock else None
    last = 0

    while True:
        worst = None

        if args.mock:
            pkt = next(source)
            if pkt.get("type") != "STROKE":
                continue
            add_stroke(pkt)
            ids = aligned_ids()
            if ids is None:
                continue
            worst = find_worst(ids)
        else:
            line = ser.readline().decode("ascii", "ignore").strip()
            if not line:
                continue
            if line.startswith("D|"):
                say("RX", line)
                b = parse_batch(line)          # compact batch from the ESP32
                if b is None:
                    continue
                worst = worst_from_batch(b)
            else:
                try:                            # single JSON stroke, older format
                    pkt = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if pkt.get("type") != "STROKE":
                    continue
                add_stroke(pkt)
                ids = aligned_ids()
                if ids is None:
                    continue
                worst = find_worst(ids)

        if worst is None or time.time() - last < 2.0:
            continue

        rower, key, value = worst

        # A real rowing deviation is a fraction of a stroke. Anything this
        # large means the batch held strokes from different moments, not a
        # rower who is out of sync. Do not alert on it.
        if key == "T" and abs(value) > MAX_PLAUSIBLE_CS:
            say(f"  skipped: timing spread {value} cs is not a stroke "
                f"deviation, the buffers are misaligned\n")
            continue
        q = make_question(rower, key, value)
        say(q)
        alert = ask_gemma(q)
        if alert is None:
            continue

        if args.json:
            out = json.dumps(make_decision(rower, key, value, alert),
                             separators=(",", ":"))
        else:
            out = make_packet(rower, key, alert)
        say("  ->", out, "\n")
        if ser:
            ser.write((out + "\n").encode())
        last = time.time()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nbye")
