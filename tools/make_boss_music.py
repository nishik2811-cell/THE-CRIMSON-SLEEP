#!/usr/bin/env python3
"""Composes and renders the Devil's fight music (assets/audio/boss_fight.ogg).

    python3 tools/make_boss_music.py [assets/audio/boss_fight.ogg]

A seamless 32-bar loop at 140 bpm in D minor (about 55 s), synthesised with
numpy and encoded with ffmpeg:
  * war drums  - deep taiko-style hits on a driving pattern, toms filling the
                 last beat of every second bar;
  * ostinato   - a low, gritty bass line (D D F E D C D A) that never stops;
  * choir pad  - dark "ah" chords (Dm - Bb - Gm - A), each with a dissonant
                 note rubbing against it;
  * strings    - a high tremolo line that enters in the second half;
  * anvil hits - a metal clang on the first beat of every 4th bar.
Filtering is done in the frequency domain (FFT) so it renders in seconds.
"""
import os
import subprocess
import sys
import tempfile
import wave

import numpy as np

DST = sys.argv[1] if len(sys.argv) > 1 else "assets/audio/boss_fight.ogg"
SR = 44100
BPM = 140
BEAT = 60.0 / BPM
BARS = 32
N = int(BARS * 4 * BEAT * SR)
rng = np.random.default_rng(666)


def note(midi):
    return 440.0 * 2 ** ((midi - 69) / 12)


def band(x, lo=None, hi=None):
    """Smooth band-pass by FFT mask (lo / hi in Hz, either may be None)."""
    X = np.fft.rfft(x)
    f = np.fft.rfftfreq(len(x), 1 / SR)
    m = np.ones_like(f)
    if hi is not None:
        m *= 1 / (1 + (f / hi) ** 4)
    if lo is not None:
        m *= 1 / (1 + (lo / np.maximum(f, 1e-3)) ** 4)
    return np.fft.irfft(X * m, len(x))


def place(buf, sound, t):
    i = int(t * SR)
    if i >= len(buf):
        return
    j = min(len(buf), i + len(sound))
    buf[i:j] += sound[: j - i]


def env(n, a, tau):
    t = np.arange(n) / SR
    return np.clip(t / a, 0, 1) * np.exp(-np.maximum(t - a, 0) / tau)


def drum(freq=55, length=0.7, drop=2.2, noise=0.25):
    n = int(length * SR)
    t = np.arange(n) / SR
    pitch = freq * (1 + drop * np.exp(-t / 0.025))
    body = np.sin(2 * np.pi * np.cumsum(pitch) / SR) * env(n, 0.002, 0.18)
    skin = band(rng.standard_normal(n), 80, 1200) * env(n, 0.001, 0.04) * noise
    return body + skin


def anvil():
    n = int(1.6 * SR)
    t = np.arange(n) / SR
    out = np.zeros(n)
    for r, a, tau in ((1, 1, 0.5), (2.76, 0.6, 0.3), (5.4, 0.4, 0.18), (8.93, 0.25, 0.1)):
        out += a * np.sin(2 * np.pi * 620 * r * t) * np.exp(-t / tau)
    return out * 0.5


def saw(freq, n):
    t = np.arange(n) / SR
    out = np.zeros(n)
    for k in range(1, 12):
        out += np.sin(2 * np.pi * freq * k * t) / k
    return out


def render():
    drums = np.zeros(N)
    bass = np.zeros(N)
    pad = np.zeros(N)
    strings = np.zeros(N)
    hits = np.zeros(N)
    big, mid, tom = drum(48, 0.9, 2.5, 0.3), drum(62, 0.6, 1.8, 0.25), drum(110, 0.35, 1.2, 0.35)
    # Drums: boom . . boom | boom . boom . (in eighths), toms on the last beat of even bars.
    pattern = [1, 0, 0, 2, 1, 0, 2, 0]
    for bar in range(BARS):
        for e, p in enumerate(pattern):
            t = (bar * 4 + e * 0.5) * BEAT
            if p == 1:
                place(drums, big, t)
            elif p == 2:
                place(drums, mid * 0.8, t)
        if bar % 2 == 1:
            for s in range(4):
                place(drums, tom * (0.6 + 0.1 * s), (bar * 4 + 3 + s * 0.25) * BEAT)
        if bar % 4 == 0:
            place(hits, anvil(), bar * 4 * BEAT)
    # Ostinato bass in eighths: D D F E D C D A (low octave), gritty and filtered.
    riff = [38, 38, 41, 40, 38, 36, 38, 33]
    step = int(0.5 * BEAT * SR)
    for i in range(BARS * 8):
        n_ = step
        tone = saw(note(riff[i % 8]), n_) * env(n_, 0.004, 0.16)
        place(bass, tone, i * 0.5 * BEAT)
    bass = band(bass, 30, 900)
    bass = np.tanh(bass * 1.8)                       # a little drive for grit
    # Choir pad: Dm - Bb - Gm - A, two bars each, with a rubbing note.
    chords = [[50, 53, 57, 62, 63], [46, 50, 53, 58, 57], [43, 50, 55, 58, 56], [45, 49, 52, 57, 58]]
    seg = int(8 * BEAT * SR)
    for c in range(BARS // 2):
        ch = chords[c % 4]
        n_ = seg
        t = np.arange(n_) / SR
        s = np.zeros(n_)
        for m in ch:
            for det in (-0.006, 0.0, 0.007):
                f = note(m) * (1 + det)
                vib = 1 + 0.004 * np.sin(2 * np.pi * 5.2 * t + m)
                ph = 2 * np.pi * np.cumsum(f * vib) / SR
                s += np.sin(ph) + 0.4 * np.sin(2 * ph) + 0.2 * np.sin(3 * ph)
        fade = np.minimum(1, np.minimum(t / 0.4, (seg / SR - t) / 0.4))
        place(pad, s * fade, c * 8 * BEAT)
    pad = band(pad, 120, 1600)
    # High string tremolo (second half): a slow falling line, tremolo in 16ths.
    line = [74, 72, 70, 69, 70, 69, 67, 69]
    for b in range(BARS // 2, BARS):
        n_ = int(4 * BEAT * SR)
        t = np.arange(n_) / SR
        f = note(line[b % 8])
        trem = 0.55 + 0.45 * np.abs(np.sin(2 * np.pi * (BPM / 60 * 2) * t))
        s = (saw(f, n_) * trem) * np.minimum(1, np.minimum(t / 0.05, (n_ / SR - t) / 0.08))
        place(strings, s, b * 4 * BEAT)
    strings = band(strings, 300, 5000)
    mix = 0.95 * drums / np.abs(drums).max() + 0.55 * bass / np.abs(bass).max() \
        + 0.35 * pad / np.abs(pad).max() + 0.22 * strings / max(1e-9, np.abs(strings).max()) \
        + 0.35 * hits / np.abs(hits).max()
    # Small room reverb (FFT convolution), wrapped so the loop stays seamless.
    ir_n = int(1.1 * SR)
    ir = rng.standard_normal(ir_n) * np.exp(-np.arange(ir_n) / (0.25 * SR))
    ir = band(ir, None, 5000)
    ir /= np.abs(ir).sum() / 6
    L = len(mix)
    wet = np.fft.irfft(np.fft.rfft(mix, 2 * L) * np.fft.rfft(ir, 2 * L), 2 * L)
    wet = wet[:L] + np.concatenate([wet[L:2 * L], np.zeros(0)])[:L]   # the tail wraps to the start
    out = 0.8 * mix + 0.25 * wet
    out = out / np.abs(out).max() * 0.9
    return out


def main():
    out = render()
    pcm = (np.clip(out, -1, 1) * 32767).astype(np.int16)
    with tempfile.TemporaryDirectory() as d:
        wav = os.path.join(d, "m.wav")
        with wave.open(wav, "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
            w.writeframes(pcm.tobytes())
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", wav, "-c:a", "libvorbis", "-q:a", "5", DST], check=True)
    print("wrote", DST, f"{len(out) / SR:.1f}s")


if __name__ == "__main__":
    main()
