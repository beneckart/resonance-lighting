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
            + " sens=9 cmis=0 rec=2 recmv=2421"
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
        self.assertEqual(row["sensor_bits"], 9)
        self.assertIsNone(row["firmware_rev"])
        self.assertIsNone(row["firmware_rev_age_ms"])
        self.assertFalse(row["class_mismatch"])
        self.assertEqual(row["recovery_state"], 2)
        self.assertEqual(row["recovery_detect_mv"], 2421)

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
            + " fw=fx-260817-example-b"
            + " prof=0 life=4 ptier=0 prog=3 nmin=0"
            + " cls=2 ledrail=1 ledr=5 ledg=40 ledb=90 ledw=0 ledn=12"
            + " sens=10 cmis=1 rec=0 recmv=65535"
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
        self.assertEqual(row["sensor_bits"], 10)
        self.assertTrue(row["class_mismatch"])
        self.assertIsNone(row["recovery_detect_mv"])
        self.assertEqual(row["firmware_rev"], "fx-260817-example-b")
        self.assertIsNotNone(row["firmware_rev_age_ms"])

    def test_fleet_view_is_the_primary_page(self):
        self.assertIn('id="fleetGrid"', dashboard.HTML)
        self.assertIn("compactPeerIds", dashboard.HTML)
        self.assertIn("batteryVisual", dashboard.HTML)
        self.assertIn("displayedLight", dashboard.HTML)
        self.assertIn("class-perimeter", dashboard.HTML)
        self.assertIn("class-uplight", dashboard.HTML)
        self.assertIn("top bar: rendered light color", dashboard.HTML)
        self.assertIn('class="tag-toggle"', dashboard.HTML)
        self.assertIn("resonanceTaggedLanterns", dashboard.HTML)
        self.assertIn("SAM-M8Q GPS", dashboard.HTML)
        self.assertIn("DS3231 RTC", dashboard.HTML)
        self.assertIn('class="anchor-badge gps"', dashboard.HTML)
        self.assertIn('class="anchor-badge rtc"', dashboard.HTML)
        self.assertIn('fetch("/api/strike"', dashboard.HTML)
        self.assertIn('fetch("/api/sleep"', dashboard.HTML)
        self.assertIn('data-cmd="B3600"', dashboard.HTML)
        self.assertIn('data-cmd="b"', dashboard.HTML)
        self.assertNotIn('data-cmd="S"', dashboard.HTML)
        self.assertIn('<details class="diagnostics">', dashboard.HTML)

    def test_dark_lease_command_validation(self):
        self.assertTrue(dashboard.valid_command("B1"))
        self.assertTrue(dashboard.valid_command("B3600"))
        self.assertTrue(dashboard.valid_command("B65535"))
        self.assertTrue(dashboard.valid_command("b"))
        self.assertFalse(dashboard.valid_command("B0"))
        self.assertFalse(dashboard.valid_command("B65536"))
        self.assertFalse(dashboard.valid_command("B1.5"))

    def test_transport_sleep_and_locate_command_validation(self):
        self.assertTrue(dashboard.valid_command("Q1"))
        self.assertTrue(dashboard.valid_command("Q96"))
        self.assertTrue(dashboard.valid_command("Q168"))
        self.assertFalse(dashboard.valid_command("Q0"))
        self.assertFalse(dashboard.valid_command("Q169"))
        self.assertTrue(dashboard.valid_command("L"))
        self.assertTrue(dashboard.valid_command("L0"))
        self.assertTrue(dashboard.valid_command("L120"))
        self.assertTrue(dashboard.valid_command("L900"))
        self.assertFalse(dashboard.valid_command("L901"))

    def test_serial_commands_are_line_terminated_for_bridge_os(self):
        class FakeSerial:
            is_open = True

            def __init__(self):
                self.writes = []

            def write(self, data):
                self.writes.append(data)

            def flush(self):
                pass

        state = dashboard.DashboardState()
        handle = FakeSerial()
        state.serial_handle = handle
        worker = dashboard.SerialWorker(state, "TEST", 115200, None, 54321)
        worker.send_commands([("UF2BE08", "maint"), ("b", "release")], 0)
        self.assertEqual(handle.writes, [b"UF2BE08\n", b"b\n"])

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
        self.assertTrue(dashboard.valid_command("TF2B7DC:1"))
        self.assertTrue(dashboard.valid_command("Tf2b7dc:0"))
        self.assertFalse(dashboard.valid_command("TF2B7DC:2"))

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

    def test_fleet_sleep_stays_addressed_and_preserves_charging(self):
        commands, skipped = dashboard.prepare_sleep_batch(
            {"targets": ["f2b7dc", "E39F1C", "F2B7DC", "E39A34"], "seconds": 28800},
            {
                "F2B7DC": {"age_ms": 800},
                "E39F1C": {"age_ms": 5100},
                "E39A34": {"age_ms": 1200},
            },
        )
        self.assertEqual(
            commands,
            [
                ("PF2B7DC:28800", "Sleep F2B7DC for 28800 s"),
                ("PE39A34:28800", "Sleep E39A34 for 28800 s"),
            ],
        )
        self.assertEqual(skipped, 1)
        self.assertTrue(all(dashboard.valid_command(cmd) for cmd, _ in commands))

    def test_fleet_sleep_rejects_broadcast_or_bad_duration(self):
        peers = {"F2B7DC": {"age_ms": 800}}
        with self.assertRaisesRegex(ValueError, "6-digit short MAC"):
            dashboard.prepare_sleep_batch(
                {"targets": ["all"], "seconds": 28800}, peers
            )
        with self.assertRaisesRegex(ValueError, "1 to 65535"):
            dashboard.prepare_sleep_batch(
                {"targets": ["F2B7DC"], "seconds": 70000}, peers
            )


if __name__ == "__main__":
    unittest.main()
