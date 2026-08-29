#!/usr/bin/env python3

import math
import unittest

import analyze_motion_trace as analyze


class AnalyzeMotionTraceTests(unittest.TestCase):
    def test_signed_swing_finds_one_axis_arc(self):
        samples = []
        for index in range(250):
            angle_deg = 20.0 * math.sin(2.0 * math.pi * index / 50.0)
            angle = math.radians(angle_deg)
            samples.append(
                {"gravity_mg": [1000 * math.sin(angle), 0, 1000 * math.cos(angle)]}
            )
        signed, summary = analyze.derive_signed_swing(samples)
        self.assertGreater(summary["principal_variance_fraction"], 0.97)
        self.assertGreater(summary["pitch_p05_p95_span_deg"], 35.0)
        period, correlation = analyze.dominant_period(signed, 25)
        self.assertAlmostEqual(period, 2.0, delta=0.08)
        self.assertGreater(correlation, 0.98)

    def test_unique_range_frames_deduplicates_25hz_samples(self):
        samples = [
            {"range_reads": 10},
            {"range_reads": 10},
            {"range_reads": 11},
            {"range_reads": 11},
        ]
        self.assertEqual(
            [row["range_reads"] for row in analyze.unique_range_frames(samples)],
            [10, 11],
        )

    def test_calm_trace_with_x_gravity_has_zero_coordinate(self):
        samples = [{"gravity_mg": [1000, 0, 0]} for _ in range(100)]
        signed, summary = analyze.derive_signed_swing(samples)
        self.assertTrue(all(abs(value) < 1e-8 for value in signed))
        self.assertEqual(summary["principal_variance_fraction"], 0.0)


if __name__ == "__main__":
    unittest.main()
