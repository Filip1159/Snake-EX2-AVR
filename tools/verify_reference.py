#!/usr/bin/env python3
"""verify_reference.py — Verify the frozen reference set.

Checks:
  1. reference/MANIFEST.sha256 exists and every file in it matches its hash.
  2. Key anchor files exist and have the expected SHA-256 (JAR, d, sounds.nok).
  3. The d and sounds.nok contents inside snakeex2.jar match the standalone copies.
  4. The JAR SHA-256 matches the frozen value from docs/reference.md.

Exit 0 on all-clear, non-zero on mismatch.
"""

import hashlib
import os
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "reference"

# Anchor checksums (from docs/reference.md, frozen 2026-10-23)
ANCHORS = {
    "snakeex2.jar": "a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40",
    "d": "6eb869cceb6d8465dc45e9ce9a12771516056206ac2c95f83e90550db46174c8",
    "sounds.nok": "1e7078a155f7284c735b594dd56acd455d4ebd15e838d7b4bf072f868f4c17ed",
}


def sha256_file(path: Path) -> str:
    """Return the SHA-256 hex digest of a file."""
    if not path.is_file():
        return ""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check_manifest(manifest_path: Path) -> tuple[int, list[str]]:
    """Verify every file in MANIFEST.sha256 exists and matches its hash."""
    if not manifest_path.is_file():
        return 1, ["ERROR: reference/MANIFEST.sha256 not found"]

    errors = []
    with open(manifest_path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(maxsplit=2)
            if len(parts) < 2:
                errors.append(f"ERROR: malformed line in MANIFEST.sha256: {line}")
                continue
            expected_hash, filename = parts[0], parts[1]
            ref_file = REF / filename
            if not ref_file.is_file():
                errors.append(f"ERROR: {filename} listed in manifest but missing")
                continue
            actual = sha256_file(ref_file)
            if actual != expected_hash:
                errors.append(
                    f"MISMATCH: {filename}  expected={expected_hash}  actual={actual}"
                )
    return len(errors), errors


def check_anchors() -> tuple[int, list[str]]:
    """Check that key anchor files exist and match their expected hashes."""
    errors = []
    for filename, expected in ANCHORS.items():
        path = REF / filename
        if not path.is_file():
            errors.append(f"ERROR: anchor {filename} missing")
            continue
        actual = sha256_file(path)
        if actual != expected:
            errors.append(
                f"ANCHOR MISMATCH: {filename}  expected={expected}  actual={actual}"
            )
    return len(errors), errors


def check_jar_internals() -> tuple[int, list[str]]:
    """Verify d and sounds.nok inside snakeex2.jar match standalone copies."""
    jar_path = REF / "snakeex2.jar"
    errors = []

    if not jar_path.is_file():
        return 1, ["ERROR: snakeex2.jar missing — cannot check internals"]

    try:
        with zipfile.ZipFile(jar_path, "r") as zf:
            names = zf.namelist()
            for member in ["d", "sounds.nok"]:
                standalone = REF / member
                if member not in names:
                    errors.append(f"ERROR: {member} not found inside JAR")
                    continue
                jar_data = zf.read(member)
                jar_hash = sha256_bytes(jar_data)
                standalone_hash = sha256_file(standalone) if standalone.is_file() else None

                if standalone_hash is None:
                    errors.append(f"ERROR: standalone {member} missing")
                    continue

                if jar_hash != standalone_hash:
                    errors.append(
                        f"JAR-INTERNAL MISMATCH: {member} inside JAR ({jar_hash}) != standalone ({standalone_hash})"
                    )
    except zipfile.BadZipFile:
        errors.append("ERROR: snakeex2.jar is not a valid ZIP/JAR")
    except Exception as e:
        errors.append(f"ERROR reading JAR: {e}")

    return len(errors), errors


def main():
    print("Snake EX2 Reference Verification")
    print("=================================")
    print()

    total_errors = 0

    # 1. Manifest check
    print("[1/3] Checking reference/MANIFEST.sha256 ...")
    n, errs = check_manifest(REF / "MANIFEST.sha256")
    total_errors += n
    if errs:
        for e in errs:
            print(f"  {e}")
    else:
        print("  OK — all manifest files verified")
    print()

    # 2. Anchor checks
    print("[2/3] Checking anchor file hashes ...")
    n, errs = check_anchors()
    total_errors += n
    if errs:
        for e in errs:
            print(f"  {e}")
    else:
        print("  OK — JAR, d, sounds.nok anchors match")
    print()

    # 3. JAR internal checks
    print("[3/3] Checking d and sounds.nok inside JAR match standalone copies ...")
    n, errs = check_jar_internals()
    total_errors += n
    if errs:
        for e in errs:
            print(f"  {e}")
    else:
        print("  OK — JAR internals match standalone copies")
    print()

    if total_errors == 0:
        print("ALL CHECKS PASSED — reference set is frozen and verified.")
        return 0
    else:
        print(f"{total_errors} ERROR(S) FOUND — reference set is NOT clean.")
        return 1


if __name__ == "__main__":
    sys.exit(main())