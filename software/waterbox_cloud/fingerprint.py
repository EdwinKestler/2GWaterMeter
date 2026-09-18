"""W(t) = w + a1 sin(2π t/T1 + p1) + a2 sin(2π t/T2 + p2), t = hour of day."""

from __future__ import annotations

import math

T1 = 24.0
T2 = 12.0


def design_row(t, t1=T1, t2=T2):
    w1 = 2.0 * math.pi * t / t1
    w2 = 2.0 * math.pi * t / t2
    return [1.0, math.sin(w1), math.cos(w1), math.sin(w2), math.cos(w2)]


def _transpose(m):
    return [list(row) for row in zip(*m)]


def _matmul(a, b):
    return [[sum(x * y for x, y in zip(row, col)) for col in zip(*b)] for row in a]


def _matvec(a, v):
    return [sum(x * y for x, y in zip(row, v)) for row in a]


def _solve(a, b):
    n = len(b)
    m = [row[:] + [b[i]] for i, row in enumerate(a)]
    for i in range(n):
        pivot = max(range(i, n), key=lambda r: abs(m[r][i]))
        m[i], m[pivot] = m[pivot], m[i]
        diag = m[i][i]
        if abs(diag) < 1e-12:
            raise ValueError("singular fingerprint fit")
        for j in range(i, n + 1):
            m[i][j] /= diag
        for r in range(n):
            if r == i:
                continue
            f = m[r][i]
            for j in range(i, n + 1):
                m[r][j] -= f * m[i][j]
    return [m[i][n] for i in range(n)]


def fit(hours, lms, t1=T1, t2=T2):
    if len(hours) < 5:
        raise ValueError("need at least 5 samples")
    x = [design_row(t, t1, t2) for t in hours]
    xt = _transpose(x)
    c = _solve(_matmul(xt, x), _matvec(xt, lms))
    w, b1, c1, b2, c2 = c
    a1 = math.hypot(b1, c1)
    a2 = math.hypot(b2, c2)
    p1 = math.atan2(c1, b1) if a1 else 0.0
    p2 = math.atan2(c2, b2) if a2 else 0.0
    return {
        "w": w,
        "a1": a1,
        "p1": p1,
        "t1": t1,
        "a2": a2,
        "p2": p2,
        "t2": t2,
        "n_samples": len(hours),
    }


def predict(params, hour):
    w = float(params["w"])
    t1 = float(params.get("t1") or T1)
    t2 = float(params.get("t2") or T2)
    a1 = float(params.get("a1") or 0.0)
    a2 = float(params.get("a2") or 0.0)
    if t1 > 0 and a1:
        w += a1 * math.sin(2.0 * math.pi * hour / t1 + float(params.get("p1") or 0.0))
    if t2 > 0 and a2:
        w += a2 * math.sin(2.0 * math.pi * hour / t2 + float(params.get("p2") or 0.0))
    return max(0.0, w)


def to_mqtt_json(params):
    return (
        '{{"w":{w:.4f},"a1":{a1:.4f},"p1":{p1:.4f},'
        '"a2":{a2:.4f},"p2":{p2:.4f},"t1":{t1:g},"t2":{t2:g}}}'
    ).format(**params)
