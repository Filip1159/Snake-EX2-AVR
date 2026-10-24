#!/usr/bin/env python3
"""extract_reference.py — Extract raw members from the original snakeex2.jar.

Usage:
  python3 tools/extract_reference.py --jar reference/snakeex2.jar --out reference [--force]

Extracts:
  - d (asset blob)
  - sounds.nok (tone data)
  - lang.* (language files)
  - icons/snicon.png (app icon)
  - MANIFEST.MF (MIDlet manifest)

Does NOT overwrite existing files unless --force is given.
Verifies the JAR SHA-256 matches the frozen value before extraction.
"""

import argparse
import hashlib
import os
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Frozen JAR SHA-256 (from docs/reference.md)
FROZEN_JAR_SHA = "a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40"

# Members to extract (JAR path -> output filename)
EXTRACT_MEMBERS = {
    "d": "d",
    "sounds.nok": "sounds.nok",
    "META-INF/MANIFEST.MF": "MANIFEST.MF",
    "icons/snicon.png": "icons/snicon.png",
}

# Language file glob (extract as-is from JAR root)
LANG_GLOB = "lang."


def sha256_file(path: Path) -> str:
    if not path.is_file():
        return ""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(
        description="Extract raw members from the original Snake EX2 JAR."
    )
    parser.add_argument(
        "--jar",
        required=True,
        type=Path,
        help="Path to the frozen snakeex2.jar",
    )
    parser.add_argument(
        "--out",
        required=True,
        type=Path,
        help="Output directory (should be reference/)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite existing files",
    )
    args = parser.parse_args()

    jar_path = args.jar.resolve()
    out_dir = args.out.resolve()

    # Verify JAR exists
    if not jar_path.is_file():
        print(f"ERROR: JAR not found: {jar_path}", file=sys.stderr)
        return 1

    # Verify JAR hash
    actual_sha = sha256_file(jar_path)
    if actual_sha != FROZEN_JAR_SHA:
        print(f"ERROR: JAR SHA-256 mismatch!", file=sys.stderr)
        print(f"  expected: {FROZEN_JAR_SHA}", file=sys.stderr)
        print(f"  actual:   {actual_sha}", file=sys.stderr)
        print("The JAR has been modified or corrupted. Cannot extract.", file=sys.stderr)
        return 1

    print(f"JAR SHA-256 verified: {actual_sha}")

    # Ensure output directory exists
    out_dir.mkdir(parents=True, exist_ok=True)

    # Open JAR and extract
    try:
        with zipfile.ZipFile(jar_path, "r") as zf:
            all_names = zf.namelist()

            # Extract explicit members
            for jar_path_member, out_filename in EXTRACT_MEMBERS.items():
                if jar_path_member not in all_names:
                    print(f"WARNING: {jar_path_member} not found in JAR")
                    continue
                out_path = out_dir / out_filename
                if out_path.exists() and not args.force:
                    print(f"SKIP (exists): {out_filename}")
                    continue
                out_path.parent.mkdir(parents=True, exist_ok=True)
                data = zf.read(jar_path_member)
                out_path.write_bytes(data)
                print(f"EXTRACTED: {out_filename} ({len(data)} bytes) <-- {jar_path_member}")

            # Extract language files
            for name in all_names:
                if name.startswith(LANG_GLOB) and name not in EXTRACT_MEMBERS:
                    out_path = out_dir / name
                    if out_path.exists() and not args.force:
                        print(f"SKIP (exists): {name}")
                        continue
                    data = zf.read(name)
                    out_path.write_bytes(data)
                    print(f"EXTRACTED: {name} ({len(data)} bytes)")

        print()
        print("Extraction complete.")
        print()
        print("Next steps:")
        print("  1. Rebuild reference/MANIFEST.sha256 if files changed:")
        print("     cd reference && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256")
        print("  2. Verify the reference set:")
        print("     python3 tools/verify_reference.py")

    except zipfile.BadZipFile:
        print(f"ERROR: {jar_path} is not a valid ZIP/JAR", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())