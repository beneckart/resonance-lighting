#!/usr/bin/env python3
"""Summarize a canopy/perimeter motion trace and optionally emit analysis CSV."""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
import statistics


def normalize(vector: list[float]) -> list[float]:
    magnitude = math.sqrt(sum(value * value for value in vector))
    if magnitude <= 1e-9:
        raise ValueError("zero gravity vector")
    return [value / magnitude for value in vector]


def dot(left: list[float], right: list[float]) -> float:
    return sum(a * b for a, b in zip(left, right))


def percentile(values: list[float], fraction: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    position = fraction * (len(ordered) - 1)
    lower = int(math.floor(position))
    upper = int(math.ceil(position))
    if lower == upper:
        return ordered[lower]
    weight = position - lower
    return ordered[lower] * (1.0 - weight) + ordered[upper] * weight


def derive_signed_swing(samples: list[dict]) -> tuple[list[float], dict]:
    vectors = [normalize([float(v) for v in row["gravity_mg"]]) for row in samples]
    mean = normalize(
        [sum(vector[axis] for vector in vectors) / len(vectors) for axis in range(3)]
    )
    covariance = [[0.0] * 3 for _ in range(3)]
    for vector in vectors:
        delta = [vector[axis] - mean[axis] for axis in range(3)]
        for row in range(3):
            for column in range(3):
                covariance[row][column] += delta[row] * delta[column]
    scale = 1.0 / max(1, len(vectors) - 1)
    covariance = [[value * scale for value in row] for row in covariance]

    axis = normalize([1.0, 0.7, 0.3])
    for _ in range(32):
        projected = [
            sum(covariance[row][column] * axis[column] for column in range(3))
            for row in range(3)
        ]
        if math.sqrt(dot(projected, projected)) <= 1e-12:
            projected = [1.0, 0.0, 0.0]
        axis = normalize(projected)
    # Make the PCA direction tangent to mean gravity. This converts the unit
    # sphere projection into a signed angular coordinate around the rest arc.
    along_mean = dot(axis, mean)
    tangent = [axis[i] - along_mean * mean[i] for i in range(3)]
    if math.sqrt(dot(tangent, tangent)) <= 1e-9:
        # A perfectly calm trace has no PCA direction. Choose a deterministic
        # axis perpendicular to mean gravity so analysis reports zero motion
        # instead of failing based on fixture mounting orientation.
        basis_index = min(range(3), key=lambda index: abs(mean[index]))
        basis = [0.0, 0.0, 0.0]
        basis[basis_index] = 1.0
        projection = dot(basis, mean)
        tangent = [basis[i] - projection * mean[i] for i in range(3)]
    tangent = normalize(tangent)
    largest = max(range(3), key=lambda index: abs(tangent[index]))
    if tangent[largest] < 0:
        tangent = [-value for value in tangent]

    signed = [
        math.degrees(math.atan2(dot(vector, tangent), dot(vector, mean)))
        for vector in vectors
    ]
    eigenvalue = dot(
        tangent,
        [
            sum(covariance[row][column] * tangent[column] for column in range(3))
            for row in range(3)
        ],
    )
    total_variance = sum(covariance[index][index] for index in range(3))
    summary = {
        "mean_gravity_unit": [round(value, 6) for value in mean],
        "principal_swing_axis": [round(value, 6) for value in tangent],
        "principal_variance_fraction": round(
            eigenvalue / total_variance if total_variance > 1e-12 else 0.0, 4
        ),
        "pitch_p05_deg": round(percentile(signed, 0.05), 3),
        "pitch_p50_deg": round(percentile(signed, 0.50), 3),
        "pitch_p95_deg": round(percentile(signed, 0.95), 3),
        "pitch_p05_p95_span_deg": round(
            percentile(signed, 0.95) - percentile(signed, 0.05), 3
        ),
    }
    return signed, summary


def dominant_period(values: list[float], hz: int) -> tuple[float | None, float | None]:
    if len(values) < hz * 3:
        return None, None
    centered = [value - statistics.fmean(values) for value in values]
    energy = sum(value * value for value in centered)
    if energy <= 1e-9:
        return None, None
    first = max(2, int(0.5 * hz))
    last = min(len(values) // 2, int(10.0 * hz))
    correlations: list[tuple[int, float]] = []
    for lag in range(first, last + 1):
        numerator = sum(
            centered[index] * centered[index + lag]
            for index in range(len(centered) - lag)
        )
        denominator = math.sqrt(
            sum(value * value for value in centered[:-lag])
            * sum(value * value for value in centered[lag:])
        )
        correlations.append((lag, numerator / denominator if denominator else 0.0))
    peaks = [
        correlations[index]
        for index in range(1, len(correlations) - 1)
        if correlations[index][1] >= correlations[index - 1][1]
        and correlations[index][1] >= correlations[index + 1][1]
    ]
    if not peaks:
        return None, None
    lag, correlation = max(peaks, key=lambda item: item[1])
    return lag / hz, correlation


def load_trace(path: Path) -> tuple[dict, list[dict], dict]:
    header: dict = {}
    footer: dict = {}
    samples: list[dict] = []
    with path.open(encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, 1):
            if not line.strip():
                continue
            row = json.loads(line)
            kind = row.get("kind")
            if kind == "capture_meta":
                header = row
            elif kind == "sample":
                samples.append(row)
            elif kind == "capture_summary":
                footer = row
            else:
                raise ValueError(f"unknown row kind at line {line_number}: {kind!r}")
    if not header or not samples:
        raise ValueError("trace requires capture metadata and at least one sample")
    return header, samples, footer


def unique_range_frames(samples: list[dict]) -> list[dict]:
    frames: list[dict] = []
    previous = None
    for sample in samples:
        sequence = int(sample.get("range_reads", 0))
        if sequence and sequence != previous:
            frames.append(sample)
            previous = sequence
    return frames


def summarize(header: dict, samples: list[dict], footer: dict) -> tuple[dict, list[float]]:
    signed, motion = derive_signed_swing(samples)
    hz = int(header.get("trace_meta", {}).get("sample_hz", 25))
    period_s, period_corr = dominant_period(signed, hz)
    frames = unique_range_frames(samples)
    range_sensor = int(samples[0].get("range_sensor", 0))
    elapsed_s = (int(samples[-1]["uptime_ms"]) - int(samples[0]["uptime_ms"])) / 1000
    summary: dict = {
        "target": header.get("target"),
        "label": header.get("label"),
        "fixture_class": header.get("trace_meta", {}).get("fixture_class"),
        "samples": len(samples),
        "sample_span_s": round(elapsed_s, 3),
        "range_frames": len(frames),
        "sequence_gaps": footer.get("sequence_gaps"),
        "motion": motion,
        "dominant_period_s": round(period_s, 3) if period_s is not None else None,
        "dominant_period_correlation": (
            round(period_corr, 4) if period_corr is not None else None
        ),
        "sway_mg_p95": round(percentile([float(r["sway_mg"]) for r in samples], 0.95), 2),
        "range_interaction_duty_pct": round(
            100 * statistics.fmean(int(r.get("range_interaction_active", 0)) for r in samples),
            2,
        ),
        "presence_duty_pct": round(
            100 * statistics.fmean(int(r.get("presence_active", 0)) for r in samples),
            2,
        ),
    }
    if range_sensor == 2 and frames:
        summary["perimeter"] = {
            "no_return_frames": sum(int(row.get("vl_no_return", 0)) for row in frames),
            "no_return_pct": round(
                100 * statistics.fmean(int(row.get("vl_no_return", 0)) for row in frames),
                2,
            ),
            "plane_valid_pct": round(
                100 * statistics.fmean(int(row.get("vl_plane_valid", 0)) for row in frames),
                2,
            ),
            "closest_mm_p50_when_valid": round(
                percentile(
                    [float(row["closest_mm"]) for row in frames if int(row["closest_mm"]) > 0],
                    0.5,
                ),
                1,
            ),
        }
    return summary, signed


def write_csv(path: Path, samples: list[dict], signed: list[float]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "seq",
        "uptime_ms",
        "signed_pitch_deg",
        "sway_mg",
        "range_sensor",
        "range_reads",
        "range_frame_ms",
        "closest_mm",
        "vl_plane_valid",
        "vl_no_return",
        "range_interaction_active",
        "presence_active",
        "presence_rising",
        "led_rail",
        "led_r",
        "led_g",
        "led_b",
        "led_w",
        "led_lit_pixels",
    ]
    with path.open("x", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for row, pitch in zip(samples, signed):
            led = list(row.get("led", [0, 0, 0, 0, 0, 0]))
            writer.writerow(
                {
                    "seq": row.get("seq"),
                    "uptime_ms": row.get("uptime_ms"),
                    "signed_pitch_deg": round(pitch, 5),
                    "sway_mg": row.get("sway_mg"),
                    "range_sensor": row.get("range_sensor"),
                    "range_reads": row.get("range_reads"),
                    "range_frame_ms": row.get("range_frame_ms"),
                    "closest_mm": row.get("closest_mm"),
                    "vl_plane_valid": row.get("vl_plane_valid"),
                    "vl_no_return": row.get("vl_no_return"),
                    "range_interaction_active": row.get("range_interaction_active"),
                    "presence_active": row.get("presence_active"),
                    "presence_rising": row.get("presence_rising"),
                    "led_rail": led[0],
                    "led_r": led[1],
                    "led_g": led[2],
                    "led_b": led[3],
                    "led_w": led[4],
                    "led_lit_pixels": led[5],
                }
            )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", help="captured motion JSONL")
    parser.add_argument("--csv", help="exclusive-create augmented analysis CSV")
    parser.add_argument("--summary-json", help="exclusive-create summary JSON")
    args = parser.parse_args()
    try:
        header, samples, footer = load_trace(Path(args.trace))
        summary, signed = summarize(header, samples, footer)
        if args.csv:
            write_csv(Path(args.csv), samples, signed)
        if args.summary_json:
            path = Path(args.summary_json)
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open("x", encoding="utf-8", newline="\n") as handle:
                json.dump(summary, handle, indent=2, sort_keys=True)
                handle.write("\n")
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        raise SystemExit(f"motion trace analysis failed: {exc}") from exc
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
