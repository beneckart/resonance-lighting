#!/usr/bin/env python3
"""Regression checks for the fleet dashboard's serial contract."""

import unittest

import net_bench_dashboard as dashboard


BASE = (
    "nb-peer id=F2B7DC seq=42 rx=40 gaps=2 pdr=0.9524 rssi=-61 "
    "bv=3.072 ima=-420 soc=1 rr=software ca=2 mode=0 dlpdr=0.998 "
    "dlrssi=-58 up=123456 age=800 sv=5.812 sma=301 sgood=1"
)


class DashboardParserTests(unittest.TestCase):
    def parse(self, line: str):
        state = dashboard.DashboardState()
        worker = dashboard.SerialWorker(state, "TEST", 115200, None, 54321)
        worker.handle_line(line)
        return state.snapshot()["peers"]["F2B7DC"]

    def test_fixture_and_render_tail(self):
        row = self.parse(
            BASE
            + " prof=0 life=4 ptier=0 prog=3 nmin=0"
            + " cls=1 ledrail=1 ledr=24 ledg=6 ledb=180 ledw=10 ledn=1"
        )
        self.assertEqual(row["profile"], 0)
        self.assertEqual(row["life_state"], 4)
        self.assertEqual(row["power_tier"], 0)
        self.assertEqual(row["active_program"], 3)
        self.assertEqual(row["fixture_class"], 1)
        self.assertTrue(row["led_rail_on"])
        self.assertEqual(
            (row["led_r"], row["led_g"], row["led_b"], row["led_w"]),
            (24, 6, 180, 10),
        )
        self.assertEqual(row["led_lit_pixels"], 1)

    def test_legacy_line_remains_valid(self):
        row = self.parse(BASE)
        self.assertEqual(row["battery_v"], 3.072)
        self.assertIsNone(row["profile"])
        self.assertIsNone(row["fixture_class"])
        self.assertIsNone(row["led_rail_on"])

    def test_short_heartbeat_preserves_rich_class_and_render_tail(self):
        state = dashboard.DashboardState()
        worker = dashboard.SerialWorker(state, "TEST", 115200, None, 54321)
        worker.handle_line(
            BASE
            + " prof=0 life=4 ptier=0 prog=3 nmin=0"
            + " cls=2 ledrail=1 ledr=5 ledg=40 ledb=90 ledw=0 ledn=12"
        )
        worker.handle_line(BASE.replace("seq=42", "seq=43"))
        row = state.snapshot()["peers"]["F2B7DC"]
        self.assertEqual(row["seq"], 43)
        self.assertEqual(row["fixture_class"], 2)
        self.assertTrue(row["led_rail_on"])
        self.assertEqual(
            (row["led_r"], row["led_g"], row["led_b"], row["led_w"]),
            (5, 40, 90, 0),
        )
        self.assertEqual(row["led_lit_pixels"], 12)

    def test_fleet_view_is_the_primary_page(self):
        self.assertIn('id="fleetGrid"', dashboard.HTML)
        self.assertIn("compactPeerIds", dashboard.HTML)
        self.assertIn("batteryVisual", dashboard.HTML)
        self.assertIn("displayedLight", dashboard.HTML)
        self.assertIn("class-perimeter", dashboard.HTML)
        self.assertIn("class-uplight", dashboard.HTML)
        self.assertIn("top bar: rendered light color", dashboard.HTML)
        self.assertIn('fetch("/api/strike"', dashboard.HTML)
        self.assertIn('<details class="diagnostics">', dashboard.HTML)

    def test_fleet_strike_stays_addressed_and_skips_stale_peers(self):
        commands, skipped = dashboard.prepare_strike_batch(
            {"targets": ["f2b7dc", "E39F1C", "F2B7DC", "E39A34"], "pulse_ms": 40},
            {
                "F2B7DC": {"age_ms": 800},
                "E39F1C": {"age_ms": 5100},
                "E39A34": {"age_ms": 1200},
            },
        )
        self.assertEqual(
            commands,
            [
                ("KF2B7DC:40", "Strike F2B7DC D7 for 40 ms"),
                ("KE39A34:40", "Strike E39A34 D7 for 40 ms"),
            ],
        )
        self.assertEqual(skipped, 1)
        self.assertTrue(all(dashboard.valid_command(cmd) for cmd, _ in commands))

    def test_fleet_strike_rejects_broadcast_or_bad_pulse(self):
        peers = {"F2B7DC": {"age_ms": 800}}
        with self.assertRaisesRegex(ValueError, "6-digit short MAC"):
            dashboard.prepare_strike_batch(
                {"targets": ["all"], "pulse_ms": 40},
                peers,
            )
        with self.assertRaisesRegex(ValueError, "5 to 300"):
            dashboard.prepare_strike_batch(
                {"targets": ["F2B7DC"], "pulse_ms": 500},
                peers,
            )


if __name__ == "__main__":
    unittest.main()
