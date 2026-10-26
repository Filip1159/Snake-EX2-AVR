#!/usr/bin/env python3
"""gen_sounds.py — Decode Nokia's `sounds.nok` into the AVR tone table (BACKLOG S1).

The original ToneDevice reads 13 length-prefixed chunks from `sounds.nok`, wraps
each in a Nokia UI `Sound(data, FORMAT_TONE)` object, then *overwrites index 12*
with a synthetic 1760 Hz / 100 ms beep.  FORMAT_TONE is the Nokia Smart
Messaging binary Ringing Tone Programming Language (RTPL), documented in the
Smart Messaging Specification §3.7/3.8.

This tool is the single source of truth for `src/assets/sounds_gen.*`.  It
verifies the frozen reference checksum, decodes the bit-packed RTPL, and writes:

  build/assets/sounds.json      decoded notes (name, Hz, ms) for auditing
  src/assets/sounds_gen.h/.cpp  compact PROGMEM tables used by the firmware

Sequence indices 0..11 are the file's first twelve chunks; index 12 is the
synthetic beep, exactly like ToneDevice.initialise().  The file's thirteenth
chunk is decoded for validation but intentionally not emitted.

Usage: python3 tools/gen_sounds.py [--in reference/sounds.nok]
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NOK_SHA = "1e7078a155f7284c735b594dd56acd455d4ebd15e838d7b4bf072f868f4c17ed"
NOK_SIZE = 479
SOUND_COUNT = 13          # playSFX(0..12)
BEEP_INDEX = 12
BEEP_HZ = 1760
BEEP_MS = 100

# RTPL tables, Smart Messaging Specification rev 2.0.0 §3.8.
BPM = [25, 28, 31, 35, 40, 45, 50, 56, 63, 70, 80, 90, 100, 112, 125, 140,
       160, 180, 200, 225, 250, 285, 320, 355, 400, 450, 500, 565, 635, 715,
       800, 900]
NOTE_NAMES = ["pause", "C", "Cis", "D", "Dis", "E", "F", "Fis", "G", "Gis",
              "A", "Ais", "H"]
# A4 = 440 Hz at scale 0.  C..Gis lie below A in the same octave (C4..H4).
SEMITONES_FROM_A = {"C": -9, "Cis": -8, "D": -7, "Dis": -6, "E": -5,
                    "F": -4, "Fis": -3, "G": -2, "Gis": -1,
                    "A": 0, "Ais": 1, "H": 2}
DURATION_BEATS = {0: 4.0, 1: 2.0, 2: 1.0, 3: 0.5, 4: 0.25, 5: 0.125}
DURATION_SPEC = {0: 1.0, 1: 1.5, 2: 1.75, 3: 2.0 / 3.0}


def fail(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


class BitReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def read(self, n):
        v = 0
        for _ in range(n):
            if self.pos >= len(self.data) * 8:
                fail("RTPL bitstream ended early")
            v = (v << 1) | ((self.data[self.pos // 8] >> (7 - (self.pos % 8))) & 1)
            self.pos += 1
        return v

    def align(self):
        while self.pos % 8:
            self.pos += 1


def note_hz(name, scale):
    if name == "pause":
        return 0
    return 440.0 * (2 ** scale) * 2 ** (SEMITONES_FROM_A[name] / 12.0)


def decode_rtpl(chunk, index):
    """Decode one RTPL tone chunk.  Returns (notes, end_bit, state)."""
    b = BitReader(chunk)
    if b.read(8) != 0x02:
        fail(f"sound {index}: command count is not 2")
    if b.read(7) != 0x25:                       # RingingToneProgramming
        fail(f"sound {index}: first command is not programming")
    # The specification's example and gnokii's unpacker align the *first*
    # command part to an octet; the Sound part follows immediately.
    b.align()
    if b.read(7) != 0x1D:                       # Sound
        fail(f"sound {index}: second command is not sound")
    song_type = b.read(3)
    if song_type != 0x01:                       # BasicSongType
        fail(f"sound {index}: unsupported song type {song_type}")
    name_len = b.read(4)
    name = bytes(b.read(8) for _ in range(name_len)).decode("latin-1")
    patterns = b.read(8)
    if patterns < 1:
        fail(f"sound {index}: no song patterns")

    state = {"tempo": 63, "scale": 2, "style": 0, "volume": 7}
    notes = []
    for p in range(patterns):
        pattern_id = b.read(3)
        if pattern_id != 0:
            fail(f"sound {index}: pattern {p} is not a new pattern header")
        b.read(2)                               # pattern part (A/B/C/D)
        b.read(4)                               # repeat count
        instructions = b.read(8)
        if instructions == 0:
            fail(f"sound {index}: pattern {p} reuses an undefined pattern")
        for i in range(instructions):
            iid = b.read(3)
            if iid == 1:                        # note
                raw_note = b.read(4)
                dur = b.read(3)
                spec = b.read(2)
                if raw_note >= len(NOTE_NAMES):
                    fail(f"sound {index}: reserved note value {raw_note}")
                if dur not in DURATION_BEATS or spec not in DURATION_SPEC:
                    fail(f"sound {index}: reserved duration {dur}/{spec}")
                note = NOTE_NAMES[raw_note]
                ms = DURATION_BEATS[dur] * DURATION_SPEC[spec] * 60000.0 / state["tempo"]
                notes.append({
                    "note": note,
                    "hz": int(round(note_hz(note, state["scale"]))),
                    "ms": int(round(ms)),
                })
            elif iid == 2:                      # scale
                state["scale"] = b.read(2)
            elif iid == 3:                      # style
                state["style"] = b.read(2)
            elif iid == 4:                      # tempo
                t = b.read(5)
                if t >= len(BPM):
                    fail(f"sound {index}: tempo index {t} out of range")
                state["tempo"] = BPM[t]
            elif iid == 5:                      # volume
                state["volume"] = b.read(4)
            else:
                fail(f"sound {index}: unsupported instruction {iid} at bit {b.pos - 3}")
        b.align()
    return notes, b.pos, state, name


def emit_header(path, notes_by_seq):
    lines = [
        "/* Auto-generated by tools/gen_sounds.py from reference/sounds.nok.",
        " * Do not edit.  Nokia RTPL decoded per Smart Messaging §3.8; index 12 is",
        " * the synthetic 1760 Hz / 100 ms beep from ToneDevice.initialise().",
        " *",
        " * SOUND_NOTE_FREQ/MS are a flat note pool; SOUND_SEQ_OFFSET/LEN index it.",
        " * A frequency of 0 is a pause.  Durations are in milliseconds.",
        " */",
        "#ifndef ASSETS_SOUNDS_GEN_H",
        "#define ASSETS_SOUNDS_GEN_H",
        "",
        "#include <stdint.h>",
        "",
        f"#define SOUND_BANK_COUNT {SOUND_COUNT}",
        f"#define SOUND_NOTE_COUNT {sum(len(n) for n in notes_by_seq)}",
        f"#define SOUND_BEEP_INDEX {BEEP_INDEX}",
        "",
        "extern const uint16_t SOUND_NOTE_FREQ[SOUND_NOTE_COUNT];",
        "extern const uint8_t  SOUND_NOTE_MS[SOUND_NOTE_COUNT];",
        "extern const uint16_t SOUND_SEQ_OFFSET[SOUND_BANK_COUNT];",
        "extern const uint8_t  SOUND_SEQ_LEN[SOUND_BANK_COUNT];",
        "",
        "#endif /* ASSETS_SOUNDS_GEN_H */",
        "",
    ]
    path.write_text("\n".join(lines))


def fmt_array(values, width, per_line, suffix=""):
    out = []
    for i in range(0, len(values), per_line):
        row = ", ".join(f"0x{v:0{width}X}" for v in values[i:i + per_line])
        out.append(f"    {row},")
    return "\n".join(out) + suffix


def emit_source(path, notes_by_seq):
    freqs = []
    durs = []
    offsets = []
    lengths = []
    for seq in notes_by_seq:
        offsets.append(len(freqs))
        lengths.append(len(seq))
        for n in seq:
            freqs.append(n["hz"])
            durs.append(n["ms"])

    lines = [
        "/* Auto-generated by tools/gen_sounds.py.  Do not edit. */",
        '#include "assets/sounds_gen.h"',
        "#include <avr/pgmspace.h>",
        "",
        "const uint16_t SOUND_NOTE_FREQ[SOUND_NOTE_COUNT] PROGMEM = {",
        fmt_array(freqs, 4, 12),
        "};",
        "",
        "const uint8_t SOUND_NOTE_MS[SOUND_NOTE_COUNT] PROGMEM = {",
        fmt_array(durs, 2, 16),
        "};",
        "",
        "const uint16_t SOUND_SEQ_OFFSET[SOUND_BANK_COUNT] PROGMEM = {",
        fmt_array(offsets, 4, 13),
        "};",
        "",
        "const uint8_t SOUND_SEQ_LEN[SOUND_BANK_COUNT] PROGMEM = {",
        fmt_array(lengths, 2, 13),
        "};",
        "",
    ]
    path.write_text("\n".join(lines))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="src", default=str(ROOT / "reference" / "sounds.nok"))
    args = ap.parse_args()

    src = Path(args.src)
    data = src.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if len(data) != NOK_SIZE or digest != NOK_SHA:
        fail(f"{src}: expected {NOK_SIZE} bytes / {NOK_SHA}, got {len(data)} / {digest}")

    chunks = []
    pos = 0
    while pos < len(data):
        if pos + 2 > len(data):
            fail("truncated chunk length")
        n = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        if pos + n > len(data):
            fail("chunk runs past end of sounds.nok")
        chunks.append(data[pos:pos + n])
        pos += n
    if len(chunks) != SOUND_COUNT:
        fail(f"expected {SOUND_COUNT} chunks, found {len(chunks)}")

    decoded = []
    for i, chunk in enumerate(chunks):
        notes, end_bit, state, name = decode_rtpl(chunk, i)
        if end_bit > len(chunk) * 8:
            fail(f"sound {i}: decoder overran the chunk")
        decoded.append({"index": i, "bytes": len(chunk), "name": name,
                        "end_bit": end_bit, "bits": len(chunk) * 8,
                        "final_state": state, "notes": notes})

    # ToneDevice overwrites the last Sound object with a synthetic beep.
    bank = [d["notes"] for d in decoded[:BEEP_INDEX]]
    bank.append([{"note": "beep", "hz": BEEP_HZ, "ms": BEEP_MS}])
    for i, seq in enumerate(bank):
        if not seq:
            fail(f"sound {i}: empty sequence")
        for n in seq:
            if n["hz"] < 0 or n["hz"] > 20000:
                fail(f"sound {i}: frequency {n['hz']} Hz out of range")
            if not 1 <= n["ms"] <= 255:
                fail(f"sound {i}: duration {n['ms']} ms does not fit uint8")

    out_dir = ROOT / "src" / "assets"
    emit_header(out_dir / "sounds_gen.h", bank)
    emit_source(out_dir / "sounds_gen.cpp", bank)

    manifest_dir = ROOT / "build" / "assets"
    manifest_dir.mkdir(parents=True, exist_ok=True)
    manifest = {
        "source": "reference/sounds.nok",
        "sha256": digest,
        "bytes": len(data),
        "bank_count": SOUND_COUNT,
        "beep_index": BEEP_INDEX,
        "beep_hz": BEEP_HZ,
        "beep_ms": BEEP_MS,
        "discarded_chunk_index": BEEP_INDEX,
        "sequences": [
            {
                "index": i,
                "notes": len(seq),
                "total_ms": sum(n["ms"] for n in seq),
                "notes_detail": seq,
            }
            for i, seq in enumerate(bank)
        ],
        "file_chunks": decoded,
    }
    (manifest_dir / "sounds.json").write_text(json.dumps(manifest, indent=1) + "\n")

    total = sum(len(s) for s in bank)
    print(f"gen_sounds: {len(chunks)} chunks / {len(data)} bytes verified")
    for i, seq in enumerate(bank):
        ms = sum(n["ms"] for n in seq)
        print(f"  SFX {i:2d}: {len(seq):2d} notes, {ms:4d} ms")
    print(f"gen_sounds: wrote src/assets/sounds_gen.{{h,cpp}} "
          f"({total} notes, {total * 3 + SOUND_COUNT * 3} B flash)")


if __name__ == "__main__":
    main()
