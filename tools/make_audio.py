#!/usr/bin/env python3
"""Converts the team's raw sounds (raw-assets/Audio, WAV) into the game's
assets/audio folder as OGG, loudness-matched per category, and removes
old sounds the game no longer uses.
    python3 tools/make_audio.py [raw-assets/Audio]"""
import os, sys, subprocess, glob

ROOT = os.path.join(os.path.dirname(__file__), '..')
RAW = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'raw-assets', 'Audio')
OUT = os.path.join(ROOT, 'assets', 'audio')

MUSIC = ['title_music', 'chase_music', 'memory_music', 'ending_morning', 'ending_waking', 'rank_music']
AMBIENCE = ['amb_house', 'amb_gallery', 'amb_library', 'amb_crawl', 'amb_crimson', 'amb_drowned',
            'drone_tension', 'drone_dread', 'wind_loop', 'drip_loop', 'rumble_loop', 'water_loop']
SFX = ['step_wood_%d' % i for i in range(1, 7)] + ['step_carpet_%d' % i for i in range(1, 5)] + \
      ['step_stone_%d' % i for i in range(1, 4)] + ['step_water_%d' % i for i in range(1, 5)] + \
      ['door_open_1', 'door_open_2', 'door_locked_1', 'door_locked_2', 'unlock', 'keys',
       'pickup_1', 'pickup_2', 'paper_1', 'paper_2', 'lever_1', 'lever_2',
       'whisper_1', 'whisper_2', 'whisper_3', 'slam_1', 'slam_2', 'slam_3', 'slam_4', 'slam_metal',
       'hushed_breath', 'scared_breath', 'moan_1', 'moan_2', 'moan_3',
       'glass_shatter_1', 'glass_shatter_2', 'glass_shatter_3', 'sting_1', 'sting_2',
       'heart_slow', 'heart_fast', 'hurt', 'gasp', 'death',
       'want_appear', 'want_1', 'want_2', 'want_3', 'voice_1', 'voice_2', 'voice_3', 'voice_4', 'drowned_lunge',
       'memory_chime', 'throw', 'ui_move', 'ui_click']
KEEP_OLD = {'scream.ogg', 'jumpscare.ogg', 'beep.wav'}   # no new equivalent

def convert(name, loud, mono=False):
    src = next((p for p in glob.glob(os.path.join(RAW, name + '.*')) if not p.endswith('.json')), None)
    if not src:
        print('MISSING', name); return None
    dst = os.path.join(OUT, name + '.ogg')
    af = 'loudnorm=I=%d:TP=-1.5:LRA=11' % loud
    cmd = ['ffmpeg', '-v', 'error', '-y', '-i', src, '-af', af, '-ar', '44100']
    cmd += ['-ac', '1'] if mono else ['-ac', '2']
    cmd += ['-c:a', 'libvorbis', '-q:a', '4', dst]
    subprocess.run(cmd, check=True)
    return os.path.basename(dst)

made = set()
for n in MUSIC: made.add(convert(n, -18))
for n in AMBIENCE: made.add(convert(n, -24))
for n in SFX: made.add(convert(n, -16, mono=True))
made.discard(None)
# remove anything in assets/audio the game no longer uses
for p in glob.glob(os.path.join(OUT, '*')):
    b = os.path.basename(p)
    if b not in made and b not in KEEP_OLD:
        os.remove(p); print('removed old', b)
print(len(made), 'sounds converted')
