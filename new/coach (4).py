#!/usr/bin/env python3
"""
coach.py  -  Vallamkali Sync Coach, simple version

The Central sends the live readings of both rowers:

    D|acc,gyro,angle|acc,gyro,angle

Python works out how far rower 2 differs from rower 1 on each measure and
finds the biggest difference. Gemma decides whether that difference means
the rowers are out of sync, or is just normal variation between two people.

Rower 1 is the stroke seat and sets the rhythm, so only rower 2 is judged.

    python coach.py --test     check the model
    python coach.py --mock     run with fake data, no hardware
    python coach.py --port /dev/ttyAMA0
"""

import argparse
import re
import sys
import time

import requests

try:
    import serial
except ImportError:
    serial = None

URL = "http://127.0.0.1:11434/api/generate"
MODEL = "gemma3:1b"

SYSTEM = """Two rowers should move together. You are told how far the second
rower differs from the first on one measure.

Two people never match exactly. Small differences are normal and must not be
alerted. A medium difference still means they are drifting apart and should
be told.

Answer N if the number is 12 or less.
Answer Y if the number is above 12.

timing 6 -> N
timing 30 -> Y
angle 5 -> N
angle 20 -> Y
power 8 -> N
power 25 -> Y

Answer only Y or N."""

# measure key -> (label for Gemma, fault digit sent to the Central)
# measure key -> (label for Gemma, fault digit sent to the Central)
MEASURES = {
    "T": ("timing", 1),      # stroke period   -> RED
    "A": ("angle",  2),      # reach           -> BLUE
    "P": ("power",  3),      # effort          -> RED
}

QUIET = False
pending = None        # verdict seen once
confirmed = None      # verdict actually sent to the bands
sess = requests.Session()


def say(*a):
    if not QUIET:
        print(*a)


def ask_gemma(question):
    """One yes/no judgement. True, False, or None if the call failed."""
    body = {
        "model": MODEL, "system": SYSTEM, "prompt": question,
        "stream": False, "keep_alive": -1,
        "format": {"type": "string", "enum": ["Y", "N"]},
        "options": {"temperature": 0, "top_k": 1, "num_predict": 3,
                    "num_ctx": 256, "seed": 0},
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
        f"{time.time()-t:.2f}s")
    return ans


def parse_line(line):
    """D|period,acc,angle|period,acc,angle -> {1:{...},2:{...}} or None"""
    parts = line.strip().split("|")
    if len(parts) < 3 or parts[0] != "D":
        return None
    out = {}
    for i, chunk in enumerate(parts[1:], start=1):
        f = chunk.split(",")
        if len(f) != 3:
            return None
        try:
            out[i] = {"T": float(f[0]),      # stroke period, centiseconds
                      "P": float(f[1]),      # peak acceleration x10
                      "A": float(f[2])}      # peak tilt, degrees
        except ValueError:
            return None
    return out


def worst(readings):
    """Biggest difference between the follower and the leader.

    Rower 1 leads. Rower 2 follows and must stay within a small deviation of
    him. So rower 2 is always the one measured, against rower 1.

    Arithmetic only. It does not decide whether the difference matters.
    """
    if 1 not in readings or 2 not in readings:
        return None
    best = None
    for k in MEASURES:
        d = round(readings[2][k] - readings[1][k])
        if best is None or abs(d) > abs(best[2]):
            best = (2, k, d)
    return best


def make_question(key, value):
    return f"{MEASURES[key][0]} {abs(value)}"


def make_packet(rower, key, alert):
    """Two digits: rower, then fault. 00 means everyone is fine."""
    return f"{rower}{MEASURES[key][1]}" if alert else "00"


# ---------------------------------------------------------------- testing

TESTS = [
    (("T", 3),   False),
    (("T", 60),  True),
    (("A", 4),   False),
    (("A", 25),  True),
    (("P", 2),   False),
    (("P", 30),  True),
]


def run_tests():
    print("warming up...")
    ask_gemma(make_question(*TESTS[0][0]))
    print()
    passed = 0
    for (key, value), want in TESTS:
        q = make_question(key, value)
        print("  " + q)
        got = ask_gemma(q)
        ok = got == want
        passed += ok
        print(f"  {'PASS' if ok else 'FAIL'}  want={'ALERT' if want else 'no alert'}\n")
    print(f"{passed}/{len(TESTS)} correct")
    if passed < len(TESTS) - 1:
        print("Try:  --model gemma3:4b")


def fake_lines():
    """Rower 2 slowly drifts out of sync."""
    import random
    n = 0
    while True:
        n += 1
        drift = min(50, 5 * n)
        yield (f"D|{100+random.randint(-3,3)},{28+random.randint(-2,2)},"
               f"{20+random.randint(-2,2)}"
               f"|{100+drift+random.randint(-3,3)},{28+random.randint(-2,2)},"
               f"{20+random.randint(-2,2)}")
        time.sleep(2.0)


# ---------------------------------------------------------------- main

def main():
    global MODEL, QUIET
    ap = argparse.ArgumentParser()
    ap.add_argument("--mock", action="store_true")
    ap.add_argument("--test", action="store_true")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--model", default=MODEL)
    ap.add_argument("--port", default="/dev/ttyAMA0")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()
    MODEL = args.model
    QUIET = args.quiet

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
    ask_gemma(make_question("T", 3))
    say("ready\n")

    source = fake_lines() if args.mock else None
    last = 0

    while True:
        if args.mock:
            line = next(source)
        else:
            line = ser.readline().decode("ascii", "ignore").strip()
            if not line:
                continue

        readings = parse_line(line)
        if readings is None:
            continue

        say("RX", line)

        if time.time() - last < 2.0:
            continue

        w = worst(readings)
        if w is None:
            say("  waiting for both rowers\n")
            continue

        rower, key, value = w
        q = make_question(key, value)
        say(" ", q)
        alert = ask_gemma(q)
        if alert is None:
            continue

        out = make_packet(rower, key, alert)

        # A verdict must repeat before it is acted on. One odd reading can
        # never flip the LEDs, so the bands stay steady instead of flickering.
        global pending, confirmed
        if out == pending:
            if out != confirmed:
                confirmed = out
                say("  ->", out, "\n")
                if ser:
                    ser.write((out + "\n").encode())
            else:
                say("  (unchanged)\n")
        else:
            pending = out
            say(f"  ({out}, waiting for it to repeat)\n")

        last = time.time()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nbye")
