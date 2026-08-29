#!/usr/bin/env python3

import json
import unittest

import capture_motion_trace as trace


class MotionTraceTests(unittest.TestCase):
    def fixture(self):
        return {
            "fixture_id": "9E5A84",
            "fw": "fx-260829-abcdef0-t",
            "motion_trace_build": True,
            "motion_trace_target": "9E5A84",
            "motion_trace_target_match": True,
            "motion_trace_capacity": 8192,
            "fixture_class": "downlight",
            "msa311_present": True,
            "msa_read_ok": True,
            "mode": 1,
            "maint_status": 1,
        }

    def test_preflight_requires_exact_target_revision_and_sensor(self):
        trace.preflight_fixture(self.fixture(), "9E5A84", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["fixture_id"] = "F2BE0C"
        with self.assertRaisesRegex(ValueError, "identity mismatch"):
            trace.preflight_fixture(bad, "9E5A84", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["motion_trace_target_match"] = False
        with self.assertRaisesRegex(ValueError, "does not match"):
            trace.preflight_fixture(bad, "9E5A84", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["msa_read_ok"] = False
        with self.assertRaisesRegex(ValueError, "MSA311"):
            trace.preflight_fixture(bad, "9E5A84", "fx-260829-abcdef0-t")

    def test_history_cursor_is_bounded_by_retained_window(self):
        meta = {"oldest_seq": 100, "newest_seq": 1000, "sample_hz": 25}
        self.assertEqual(trace.history_cursor(meta, 10), (750, 1000))
        self.assertEqual(trace.history_cursor(meta, 300), (99, 1000))

    def test_parse_trace_ndjson(self):
        text = "\n".join(
            [
                json.dumps({"kind": "meta", "newest_seq": 2}),
                json.dumps({"kind": "sample", "seq": 2}),
            ]
        )
        meta, samples = trace.parse_trace_ndjson(text)
        self.assertEqual(meta["newest_seq"], 2)
        self.assertEqual(samples[0]["seq"], 2)


if __name__ == "__main__":
    unittest.main()
