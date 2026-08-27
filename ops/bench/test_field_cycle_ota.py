import unittest

import field_cycle_ota as ota


class PeerRejoinEvidenceTests(unittest.TestCase):
    def test_unique_revision_needs_fresh_expected_heartbeat(self):
        baseline = {"firmware_rev": "fx-old", "uptime_ms": 9000, "seq": 20}
        peer = {"firmware_rev": "fx-new", "uptime_ms": 8000, "seq": 25, "age_ms": 50}
        self.assertTrue(ota.peer_rejoin_matches(peer, baseline, "fx-new", 5000)[0])

    def test_same_revision_rejects_cached_state(self):
        baseline = {"firmware_rev": "dev-local", "uptime_ms": 9000, "seq": 20}
        peer = {"firmware_rev": "dev-local", "uptime_ms": 12000, "seq": 24, "age_ms": 50}
        ok, reason = ota.peer_rejoin_matches(peer, baseline, "dev-local", 5000)
        self.assertFalse(ok)
        self.assertIn("no post-job", reason)

    def test_same_revision_accepts_uptime_reset(self):
        baseline = {"firmware_rev": "dev-local", "uptime_ms": 900000, "seq": 500}
        peer = {"firmware_rev": "dev-local", "uptime_ms": 31000, "seq": 12, "age_ms": 50}
        self.assertTrue(ota.peer_rejoin_matches(peer, baseline, "dev-local", 5000)[0])

    def test_same_revision_without_baseline_fails_closed(self):
        peer = {"firmware_rev": "dev-local", "uptime_ms": 31000, "seq": 12, "age_ms": 50}
        ok, reason = ota.peer_rejoin_matches(peer, None, "dev-local", 5000)
        self.assertFalse(ok)
        self.assertIn("no pre-job", reason)

    def test_stale_or_wrong_revision_is_rejected(self):
        baseline = {"firmware_rev": "fx-old", "uptime_ms": 9000, "seq": 20}
        stale = {"firmware_rev": "fx-new", "uptime_ms": 100, "seq": 1, "age_ms": 6000}
        wrong = {"firmware_rev": "fx-old", "uptime_ms": 100, "seq": 1, "age_ms": 10}
        self.assertFalse(ota.peer_rejoin_matches(stale, baseline, "fx-new", 5000)[0])
        self.assertFalse(ota.peer_rejoin_matches(wrong, baseline, "fx-new", 5000)[0])


if __name__ == "__main__":
    unittest.main()
