#!/usr/bin/env python3
"""Convert capture logs into the canonical pairwise-RSSI JSONL contract.

Usage:
  ./locate_ingest.py --net-bench ops/bench/data/ca/<log>.jsonl ... --out pairwise.jsonl
  ./locate_ingest.py --pairwise-dump <future-firmware-dump>.jsonl --out pairwise.jsonl

The net_bench adapter maps the existing star-topology bench logs (master <-> peer
only) onto contract rows -- a format smoke test; a star cannot feed a full
localization. The real input will be a firmware neighbor-table dump where every
device reports per-neighbor RSSI (see ops/locate/README.md for the contract and
TODO.md for the firmware work item).
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from locate.io_jsonl import from_net_bench, read_pairwise, write_pairwise  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--net-bench", nargs="*", default=[],
                    help="net_bench star logs (peer rows with rssi_dbm/dl_rssi_dbm)")
    ap.add_argument("--pairwise-dump", nargs="*", default=[],
                    help="already-contract-shaped rows; pass through with validation")
    ap.add_argument("--master-id", default="MASTER")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    rows = []
    if args.net_bench:
        rows.extend(from_net_bench(args.net_bench, master_id=args.master_id))
    for path in args.pairwise_dump:
        for r in read_pairwise(path):
            if "tx" in r and "rx" in r and "rssi_dbm" in r:
                rows.append(r)
    if not rows:
        sys.exit("no usable rows found")
    write_pairwise(args.out, rows)
    ids = sorted({r["tx"] for r in rows} | {r["rx"] for r in rows})
    print(f"wrote {len(rows)} rows, {len(ids)} device ids -> {args.out}")
    print("ids:", ", ".join(ids[:20]) + (" ..." if len(ids) > 20 else ""))


if __name__ == "__main__":
    main()
