import unittest

from storage_census import summarize


def peer(elapsed, age, pid="ABCDEF", **fields):
    return dict(src="peer", peer_id=pid, elapsed_s=elapsed, age_ms=age,
                battery_v=3.2, **fields)


class StorageCensusTest(unittest.TestCase):
    def test_repeated_cached_snapshots_never_count_as_live(self):
        result = summarize([peer(10, 20000), peer(970, 980000)], ["ABCDEF"])
        self.assertEqual(result["cached_count"], 1)
        self.assertEqual(result["fresh_count"], 0)
        self.assertEqual(result["expected_not_observed"], ["ABCDEF"])

    def test_one_fresh_wake_counts_even_after_peer_returns_to_sleep(self):
        result = summarize([peer(20, 1000), peer(980, 961000)], ["ABCDEF"])
        self.assertEqual(result["fresh_count"], 1)
        self.assertTrue(result["full_16_minute_window"])
        self.assertEqual(result["peers"][0]["age_at_end_s"], 961)

    def test_boundary_margin_and_missing_age_fail_closed(self):
        result = summarize([peer(2, 100), peer(30, None)])
        self.assertEqual(result["fresh_count"], 0)

    def test_truncated_tail_preserves_only_prior_in_run_fields(self):
        result = summarize([peer(0, 100, profile=0),
                            peer(20, 1000, power_tier=3),
                            peer(40, 1000, power_tier=None)])
        p = result["peers"][0]
        self.assertIsNone(p["profile"])
        self.assertEqual(p["power_tier"], 3)
        self.assertEqual(p["power_tier_age_s"], 21)

    def test_unexpected_ids_are_separate_from_expected_missing(self):
        result = summarize([peer(20, 0)], ["123456"])
        self.assertEqual(result["unexpected_observed"], ["ABCDEF"])
        self.assertEqual(result["expected_seen_count"], 0)

    def test_resumed_segments_do_not_invent_continuous_coverage(self):
        a = peer(500, 0)
        a.update(segment_index=1, segment_started_utc="2026-09-12T00:00:00+00:00",
                 ts_utc="2026-09-12T00:08:20+00:00")
        b = peer(500, 0)
        b.update(segment_index=2, segment_started_utc="2026-09-12T00:10:00+00:00",
                 ts_utc="2026-09-12T00:18:20+00:00")
        result = summarize([a, b])
        self.assertEqual(result["duration_s"], 1100)
        self.assertFalse(result["full_16_minute_window"])
        self.assertEqual(result["peers"][0]["last_heard_elapsed_s"], 1100)

    def test_very_low_reports_are_exposed_even_when_outside_plausibility_filter(self):
        row = peer(20, 0)
        row["battery_v"] = 0.2
        result = summarize([row])
        self.assertEqual(result["reported_0_to_0_6v"], ["ABCDEF"])
        self.assertIsNone(result["battery_min_v"])


if __name__ == "__main__":
    unittest.main()
