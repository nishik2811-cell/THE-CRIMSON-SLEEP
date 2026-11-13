#!/usr/bin/env python3
"""Synthesises the final fight's sounds and the Devil's death music.

    python3 tools/make_combat_sfx.py [assets/audio]

Everything is built from scratch with numpy (sine partials, filtered noise,
envelopes, a small reverb) and written as .ogg with ffmpeg:

  clang_1/2.ogg        sword BLOCKS scythe: bright inharmonic metal ring + spark hiss
  sword_swing_1/2.ogg  the hero's blade cutting the air (fast rising whoosh)
  sword_hit_1/2.ogg    the blade biting the Devil: heavy thud + crunch
  scythe_swing.ogg     the scythe coming round: long, low, ragged whoosh
  scythe_cut.ogg       the scythe catching the hero: sharp slash + wet tear
  spider_squish_1/2    a stomped spider: chitin crunching, then a small wet squish
  devil_death.ogg      ~10 s: a rising dissonant swell and a roar while he
                       convulses, a crash when he bursts, then the dark
                       resolves into a slow, warm major chord with shimmer
"""
import os
import subprocess
import sys
import tempfile
import wave

import numpy as np

OUT = sys.argv[1] if len(sys.argv) > 1 else "assets/audio"
SR = 44100
rng = np.random.default_rng(1953)


def t_axis(sec):
    return np.arange(int(sec * SR)) / SR


def env(n, attack, decay_tau):
    t = np.arange(n) / SR
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    return a * np.exp(-np.maximum(t - attack, 0) / decay_tau)


def lowpass(x, cutoff):
    # one-pole low-pass (cutoff in Hz), run in both directions for no phase lag
    a = np.exp(-2 * np.pi * cutoff / SR)
    y = np.empty_like(x)
    acc = 0.0
    for i in range(len(x)):
        acc = (1 - a) * x[i] + a * acc
        y[i] = acc
    acc = 0.0
    for i in range(len(y) - 1, -1, -1):
        acc = (1 - a) * y[i] + a * acc
        y[i] = acc
    return y


def highpass(x, cutoff):
    return x - lowpass(x, cutoff)


def sweep_noise(sec, f0, f1, q=0.5):
    """Band of noise whose centre slides from f0 to f1 (a whoosh)."""
    n = int(sec * SR)
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    blocks = 24
    for b in range(blocks):
        lo, hi = b * n // blocks, (b + 1) * n // blocks
        f = f0 + (f1 - f0) * (b + 0.5) / blocks
        seg = noise[max(0, lo - 2000):hi]
        seg = highpass(lowpass(seg, f * (1 + q)), f * (1 - q * 0.8))
        out[lo:hi] = seg[-(hi - lo):]
    return out


def reverb(x, wet=0.3, length=1.2):
    n = int(length * SR)
    ir = rng.standard_normal(n) * np.exp(-np.arange(n) / (SR * length / 5))
    ir = lowpass(ir, 4000)
    ir /= np.abs(ir).sum() / 8
    y = np.convolve(x, ir)
    dry = np.concatenate([x, np.zeros(len(y) - len(x))])
    return dry * (1 - wet) + y * wet


def norm(x, peak=0.89):
    m = np.abs(x).max()
    return x * (peak / m) if m > 0 else x


def save(name, x):
    x = norm(x)
    pcm = (np.clip(x, -1, 1) * 32767).astype(np.int16)
    with tempfile.TemporaryDirectory() as d:
        wav = os.path.join(d, "s.wav")
        with wave.open(wav, "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
            w.writeframes(pcm.tobytes())
        dst = os.path.join(OUT, name)
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", wav, "-c:a", "libvorbis", "-q:a", "5", dst], check=True)
    print("wrote", dst, f"{len(x) / SR:.2f}s")


def clang(seed_shift):
    t = t_axis(1.6)
    n = len(t)
    base = 820 + 70 * seed_shift
    partials = [(1.0, 1.0, 0.55), (2.76, 0.6, 0.35), (5.40, 0.45, 0.22), (8.93, 0.3, 0.14), (13.34, 0.2, 0.08)]
    x = np.zeros(n)
    for ratio, amp, tau in partials:
        f = base * ratio * (1 + 0.002 * seed_shift)
        x += amp * np.sin(2 * np.pi * f * t + rng.uniform(0, 6)) * np.exp(-t / tau)
    hit = highpass(rng.standard_normal(n), 3000) * env(n, 0.001, 0.03) * 1.2   # the spark hiss
    thump = np.sin(2 * np.pi * 180 * t) * env(n, 0.001, 0.05) * 0.6
    return reverb(x + hit + thump, 0.25, 1.0)


def sword_swing(seed_shift):
    x = sweep_noise(0.32, 700 + 100 * seed_shift, 3600, 0.6)
    n = len(x)
    e = np.sin(np.linspace(0, np.pi, n)) ** 1.5
    return np.concatenate([x * e, np.zeros(int(0.05 * SR))])


def sword_hit(seed_shift):
    t = t_axis(0.9)
    n = len(t)
    thud = np.sin(2 * np.pi * (70 + 15 * seed_shift) * t * (1 + 0.6 * np.exp(-t / 0.03))) * env(n, 0.002, 0.16)
    crunch = lowpass(rng.standard_normal(n), 2200) * env(n, 0.001, 0.07) * 1.4
    tear = highpass(rng.standard_normal(n), 1500) * env(n, 0.004, 0.12) * 0.5
    return reverb(thud * 1.4 + crunch + tear, 0.2, 0.8)


def scythe_swing():
    x = sweep_noise(0.7, 260, 1500, 0.7)
    n = len(x)
    e = np.sin(np.linspace(0, np.pi, n)) ** 2
    rattle = np.sin(2 * np.pi * 34 * np.arange(n) / SR) * 0.25 + 1
    return reverb(x * e * rattle, 0.3, 1.0)


def scythe_cut():
    t = t_axis(0.8)
    n = len(t)
    slash = sweep_noise(0.8, 4200, 1800, 0.5) * env(n, 0.002, 0.09) * 1.5
    wet = lowpass(rng.standard_normal(n), 900) * env(n, 0.01, 0.18) * 0.9
    thump = np.sin(2 * np.pi * 95 * t) * env(n, 0.001, 0.1) * 0.8
    return reverb(slash + wet + thump, 0.18, 0.7)


def spider_squish(seed_shift):
    t = t_axis(0.5)
    n = len(t)
    x = np.zeros(n)
    # chitin cracking: a handful of tiny sharp clicks in quick succession
    for k in range(6 + seed_shift):
        at = int((0.004 + k * 0.012 + rng.uniform(0, 0.006)) * SR)
        m = int(0.006 * SR)
        click = highpass(rng.standard_normal(m), 2500) * np.exp(-np.arange(m) / (0.0015 * SR))
        x[at:at + m] += click * rng.uniform(0.5, 1.0)
    # the wet squish under the boot: low, wobbling, smeared noise
    squish = lowpass(rng.standard_normal(n), 700) * env(n, 0.02, 0.09)
    squish *= 1 + 0.6 * np.sin(2 * np.pi * (28 + 6 * seed_shift) * t)
    x += squish * 1.3
    return reverb(x, 0.12, 0.4)


def devil_death():
    total = 10.5
    t = t_axis(total)
    n = len(t)
    out = np.zeros(n)
    # 1) 0 - 2.8 s: he convulses - a low drone and a dissonant cluster
    #    swelling up, with a growl underneath.
    sw = np.clip(t / 2.8, 0, 1) ** 2 * (t < 2.85)
    for f in (55, 58.3, 82.4, 87.3, 116.5):
        out += np.sin(2 * np.pi * f * t + 0.3 * np.sin(2 * np.pi * 5.5 * t)) * sw * 0.35
    growl = lowpass(rng.standard_normal(n), 300) * sw * 1.4
    out += growl
    # 2) 2.8 s: the burst - a crash and a deep boom.
    tb = np.maximum(t - 2.8, 0)
    burst = (t >= 2.8)
    out += highpass(rng.standard_normal(n), 800) * np.exp(-tb / 0.6) * burst * 1.6
    out += np.sin(2 * np.pi * 42 * tb * (1 + 1.5 * np.exp(-tb / 0.08))) * np.exp(-tb / 1.2) * burst * 1.8
    # 3) 3.4 s on: the dark lifts - a slow warm D-major chord (choir-like:
    #    detuned saws softened by a low-pass), and a high shimmer as the souls rise.
    tc = np.maximum(t - 3.4, 0)
    pad_env = np.clip(tc / 2.5, 0, 1) * np.clip((total - t) / 2.0, 0, 1) * (t >= 3.4)
    chord = (146.83, 220.0, 293.66, 369.99, 440.0)
    pad = np.zeros(n)
    for f in chord:
        for det in (-0.004, 0.0, 0.005):
            ph = 2 * np.pi * f * (1 + det) * t
            pad += (np.sin(ph) + 0.35 * np.sin(2 * ph) + 0.15 * np.sin(3 * ph)) * (1 + 0.05 * np.sin(2 * np.pi * 4.8 * t))
    out += lowpass(pad, 1800) * pad_env * 0.16
    shimmer = np.zeros(n)
    for f in (1174.7, 1479.98, 1760.0):
        shimmer += np.sin(2 * np.pi * f * t) * (0.5 + 0.5 * np.sin(2 * np.pi * 0.7 * t + f))
    out += shimmer * np.clip((t - 5.0) / 2.0, 0, 1) * np.clip((total - t) / 2.0, 0, 1) * 0.05
    return reverb(out, 0.35, 2.4)


def main():
    os.makedirs(OUT, exist_ok=True)
    for i in (1, 2):
        save(f"clang_{i}.ogg", clang(i))
        save(f"sword_swing_{i}.ogg", sword_swing(i))
        save(f"sword_hit_{i}.ogg", sword_hit(i))
        save(f"spider_squish_{i}.ogg", spider_squish(i))
    save("scythe_swing.ogg", scythe_swing())
    save("scythe_cut.ogg", scythe_cut())
    save("devil_death.ogg", devil_death())


if __name__ == "__main__":
    main()
