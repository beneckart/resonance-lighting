#!/usr/bin/env python3

import unittest

import capture_sentinel_trace as trace


class SentinelTraceTests(unittest.TestCase):
    def fixture(self):
        return {
            "fixture_id": "A1B2C3",
            "fw": "fx-260829-abcdef0-t",
            "sentinel_trace_build": True,
            "sentinel_trace_target": "A1B2C3",
            "sentinel_trace_target_match": True,
            "sentinel_trace_phase": "retrieval",
            "sentinel_trace_capacity": 4096,
            "fixture_class": "perimeter",
            "mode": 1,
            "maint_status": 1,
        }

    def samples(self):
        return [
            {
                "kind": "sample",
                "seq": 1,
                "phase": "baseline-a",
                "radio_on": 0,
                "sensor_rail_on": 0,
                "power_flags": 1,
                "battery_ma": -50,
                "supply_ma": 0,
                "vl_reads": 0,
            },
            {
                "kind": "sample",
                "seq": 2,
                "phase": "tof-active",
                "radio_on": 0,
                "sensor_rail_on": 1,
                "power_flags": 1,
                "battery_ma": -70,
                "supply_ma": 0,
                "vl_reads": 10,
                "presence_rising": 1,
            },
            {
                "kind": "sample",
                "seq": 3,
                "phase": "tof-active",
                "radio_on": 0,
                "sensor_rail_on": 1,
                "power_flags": 1,
                "battery_ma": -72,
                "supply_ma": 0,
                "vl_reads": 15,
            },
            {
                "kind": "sample",
                "seq": 4,
                "phase": "baseline-b",
                "radio_on": 0,
                "sensor_rail_on": 0,
                "power_flags": 1,
                "battery_ma": -52,
                "supply_ma": 0,
                "vl_reads": 15,
            },
        ]

    def test_preflight_requires_exact_complete_perimeter(self):
        trace.preflight_fixture(self.fixture(), "A1B2C3", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["sentinel_trace_phase"] = "tof-active"
        with self.assertRaisesRegex(ValueError, "not complete"):
            trace.preflight_fixture(bad, "A1B2C3", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["fixture_class"] = "downlight"
        with self.assertRaisesRegex(ValueError, "not perimeter"):
            trace.preflight_fixture(bad, "A1B2C3", "fx-260829-abcdef0-t")
        bad = self.fixture()
        bad["sentinel_trace_capacity"] = 1024
        with self.assertRaisesRegex(ValueError, "too small"):
            trace.preflight_fixture(bad, "A1B2C3", "fx-260829-abcdef0-t")

    def test_campaign_validation_and_summary(self):
        rows = self.samples()
        trace.validate_campaign(rows)
        summary = trace.phase_summary(rows)
        self.assertEqual(summary["tof-active"]["battery_ma_mean"], -71.0)
        self.assertEqual(summary["tof-active"]["presence_edges"], 1)
        self.assertEqual(summary["tof-active"]["vl_read_delta"], 5)

    def test_campaign_rejects_radio_or_rail_leak(self):
        rows = self.samples()
        rows[1]["radio_on"] = 1
        with self.assertRaisesRegex(ValueError, "radio was on"):
            trace.validate_campaign(rows)
        rows = self.samples()
        rows[0]["sensor_rail_on"] = 1
        with self.assertRaisesRegex(ValueError, "sensor rail"):
            trace.validate_campaign(rows)


if __name__ == "__main__":
    unittest.main()
