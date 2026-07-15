#!/usr/bin/env python3
"""Run the ops/locate test suite with stdlib only (pytest-compatible modules).

Usage: ./locate_selftest.py [-k pattern] [-v]

Imports every tests/test_*.py module and runs each test_* function. Exits
nonzero on any failure. The modules are plain-assert and pytest-compatible, so
`pytest ops/locate/tests` works identically if pytest is ever installed.
"""

import argparse
import importlib
import os
import sys
import time
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-k", default="", help="only run tests whose name contains this substring")
    ap.add_argument("-v", action="store_true", help="print each test as it runs")
    args = ap.parse_args()

    test_files = sorted(
        f[:-3] for f in os.listdir(os.path.join(HERE, "tests"))
        if f.startswith("test_") and f.endswith(".py")
    )
    n_pass = n_fail = 0
    failures = []
    t0 = time.time()
    for mod_name in test_files:
        mod = importlib.import_module(f"tests.{mod_name}")
        for attr in sorted(dir(mod)):
            if not attr.startswith("test_"):
                continue
            name = f"{mod_name}.{attr}"
            if args.k and args.k not in name:
                continue
            try:
                if args.v:
                    print(f"  {name} ...", end=" ", flush=True)
                getattr(mod, attr)()
                n_pass += 1
                if args.v:
                    print("ok")
            except Exception:
                n_fail += 1
                failures.append((name, traceback.format_exc()))
                if args.v:
                    print("FAIL")

    dt = time.time() - t0
    for name, tb in failures:
        print(f"\nFAIL {name}\n{tb}")
    print(f"\n{n_pass} passed, {n_fail} failed in {dt:.1f}s")
    sys.exit(1 if n_fail else 0)


if __name__ == "__main__":
    main()
