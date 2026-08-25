#!/usr/bin/env python3

import unittest

import rtc_commission as rtc


class RtcCommissionTests(unittest.TestCase):
    def fixture(self):
        return {
            "fixture_id": "9F0E7C",
            "fw": "fx-test-b",
            "mode": 1,
            "maint_status": 1,
            "ds3231_present": True,
            "battery_v": 3.31,
            "supply_v": 0,
            "supply_ma": 0,
            "supply_good": False,
        }

    def test_preflight_requires_exact_identity_revision_and_rtc(self):
        rtc.preflight_fixture(self.fixture(), "9F0E7C", "fx-test-b")
        bad = self.fixture()
        bad["fixture_id"] = "9F26C0"
        with self.assertRaisesRegex(ValueError, "identity mismatch"):
            rtc.preflight_fixture(bad, "9F0E7C", "fx-test-b")
        bad = self.fixture()
        bad["ds3231_present"] = False
        with self.assertRaisesRegex(ValueError, "does not report"):
            rtc.preflight_fixture(bad, "9F0E7C", "fx-test-b")

    def test_preflight_refuses_unsafe_power(self):
        bad = self.fixture()
        bad["battery_v"] = 2.3
        with self.assertRaisesRegex(ValueError, "unsafe power"):
            rtc.preflight_fixture(bad, "9F0E7C", "fx-test-b")

    def test_gps_reference_must_be_valid_and_fresh(self):
        state = {
            "time_sources": {
                "8EB508": {
                    "utc_s": 1787631606,
                    "sub_ms": 100,
                    "source": 1,
                    "valid": True,
                    "date_valid": True,
                    "gps_age_ms": 300,
                    "observation_age_ms": 600,
                }
            }
        }
        self.assertEqual(rtc.gps_now_ms(state), 1787631607000)
        self.assertEqual(rtc.commission_utc_s(state), 1787631608)
        state["time_sources"]["8EB508"]["observation_age_ms"] = 5001
        with self.assertRaisesRegex(ValueError, "no fresh"):
            rtc.gps_now_ms(state)


if __name__ == "__main__":
    unittest.main()
