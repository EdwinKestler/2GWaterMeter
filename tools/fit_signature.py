#!/usr/bin/env python3
"""CSV helper. Live fitting is software/waterbox_cloud (Docker worker).

Input CSV: hour,lm   (hour in 0-24, lm in L/min)
Prints MQTT JSON for waterbox/<IMEI>/fp
"""
from __future__ import print_function

import csv
import math
import sys

T1 = 24.0
T2 = 12.0


def load_csv(path):
    hours, lms = [], []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            hours.append(float(row["hour"]))
            lms.append(float(row["lm"]))
    return hours, lms


def design_row(t):
    w1 = 2.0 * math.pi * t / T1
    w2 = 2.0 * math.pi * t / T2
    return [1.0, math.sin(w1), math.cos(w1), math.sin(w2), math.cos(w2)]


def transpose(m):
    return [list(row) for row in zip(*m)]


def matmul(a, b):
    out = []
    for row in a:
        out.append([sum(x * y for x, y in zip(row, col)) for col in zip(*b)])
    return out


def matvec(a, v):
    return [sum(x * y for x, y in zip(row, v)) for row in a]


def solve(a, b):
    n = len(b)
    m = [row[:] + [b[i]] for i, row in enumerate(a)]
    for i in range(n):
        pivot = max(range(i, n), key=lambda r: abs(m[r][i]))
        m[i], m[pivot] = m[pivot], m[i]
        diag = m[i][i]
        if abs(diag) < 1e-12:
            raise SystemExit("singular fit; need more varied hours")
        for j in range(i, n + 1):
            m[i][j] /= diag
        for r in range(n):
            if r == i:
                continue
            f = m[r][i]
            for j in range(i, n + 1):
                m[r][j] -= f * m[i][j]
    return [m[i][n] for i in range(n)]


def fit(hours, lms):
    x = [design_row(t) for t in hours]
    xt = transpose(x)
    xtx = matmul(xt, x)
    xty = matvec(xt, lms)
    c = solve(xtx, xty)
    w, b1, c1, b2, c2 = c
    a1 = math.hypot(b1, c1)
    a2 = math.hypot(b2, c2)
    p1 = math.atan2(c1, b1) if a1 else 0.0
    p2 = math.atan2(c2, b2) if a2 else 0.0
    return w, a1, p1, a2, p2


def main():
    if len(sys.argv) < 2:
        print("usage: fit_signature.py readings.csv", file=sys.stderr)
        print("csv header: hour,lm", file=sys.stderr)
        sys.exit(1)
    hours, lms = load_csv(sys.argv[1])
    if len(hours) < 5:
        raise SystemExit("need at least 5 rows")
    w, a1, p1, a2, p2 = fit(hours, lms)
    print(
        '{{"w":{:.4f},"a1":{:.4f},"p1":{:.4f},"a2":{:.4f},"p2":{:.4f},"t1":24,"t2":12}}'.format(
            w, a1, p1, a2, p2
        )
    )


if __name__ == "__main__":
    main()
