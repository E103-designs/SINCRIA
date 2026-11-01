#!/usr/bin/env python3
"""Run the actual SINCRIA sketch with simulated I/O and ASan/UBSan."""
from pathlib import Path
import os, subprocess, tempfile

root = Path(__file__).resolve().parents[1]
tests = root / "firmware/SINCRIA_Scansione_Righe/tests"
cxx = os.environ.get("CXX", "clang++")
with tempfile.TemporaryDirectory(prefix="sincria-tests-") as build:
    for profile in (1, 0):
        for suite in ("legacy", "handheld"):
            binary = Path(build) / f"{suite}-{profile}"
            subprocess.run([cxx, "-std=c++11", f"-DSINCRIA_INVERTED_INPUTS={profile}",
                "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer", "-I", str(tests),
                str(tests / f"test_{suite}.cpp"), "-o", str(binary)], check=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
            print(f"{suite}, input profile {profile}: {result.stdout.strip()}", flush=True)
