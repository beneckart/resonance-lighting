#!/usr/bin/env python3
"""Host-only tests for fleet OTA target policy."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import fleet_dashboard_ota as ota


REGISTRY_HEADER = "fixture_id,mac,role\n"


class SpecialTargetPolicyTests(unittest.TestCase):
    def registry(self, body: str) -> tuple[tempfile.TemporaryDirectory, Path]:
        temp = tempfile.TemporaryDirectory()
        path = Path(temp.name) / "registry.csv"
        path.write_text(REGISTRY_HEADER + body, encoding="utf-8")
        return temp, path

    def test_normal_target_needs_no_acknowledgement(self):
        temp, registry = self.registry("ABC123,00:00:00:AB:C1:23,downlight\n")
        self.addCleanup(temp.cleanup)
        ota.validate_special_targets(["ABC123"], [], registry)

    def test_magic_wand_requires_exact_acknowledgement(self):
        temp, registry = self.registry("F40344,68:EE:8F:F4:03:44,magic_wand\n")
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "--allow-special-target F40344"):
            ota.validate_special_targets(["F40344"], [], registry)
        ota.validate_special_targets(["F40344"], ["f40344"], registry)

    def test_magic_wand_must_be_only_target(self):
        temp, registry = self.registry(
            "F40344,68:EE:8F:F4:03:44,magic_wand\n"
            "ABC123,00:00:00:AB:C1:23,downlight\n"
        )
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "must be the only OTA target"):
            ota.validate_special_targets(
                ["F40344", "ABC123"], ["F40344"], registry
            )

    def test_acknowledgement_must_name_a_target(self):
        temp, registry = self.registry("ABC123,00:00:00:AB:C1:23,downlight\n")
        self.addCleanup(temp.cleanup)
        with self.assertRaisesRegex(SystemExit, "names a non-target"):
            ota.validate_special_targets(["ABC123"], ["F40344"], registry)


class FleetWorkflowTests(unittest.TestCase):
    def args(self, **overrides):
        values = {
            "gather_cadence": "ordinary",
            "ordinary_sleep_s": 120.0,
            "protect_sleep_s": 900.0,
            "gather_margin_s": 30.0,
            "discovery_timeout": None,
            "campaign_status_timeout": 8.0,
            "dashboard_url": "http://127.0.0.1:8765",
        }
        values.update(overrides)
        return SimpleNamespace(**values)

    def test_gather_timing_spans_selected_cadence(self):
        self.assertEqual(ota.gather_timing(self.args()), (150.0, 168))
        self.assertEqual(
            ota.gather_timing(self.args(gather_cadence="protect")),
            (930.0, 948),
        )
        with self.assertRaisesRegex(SystemExit, "cannot span ordinary cadence"):
            ota.gather_timing(self.args(discovery_timeout=149.0))

    def test_campaign_loads_roster_then_requires_structured_ack(self):
        emitted = []

        class Ledger:
            def emit(self, phase, event, target=None, **kwargs):
                emitted.append(((phase, event, target), kwargs))

        status = {
            "job_id": "12AB34CD",
            "phase": ota.MAINT_PHASE_GATHER,
            "active": True,
            "target_count": 2,
            "cycle_ms": 20,
        }
        with mock.patch.object(ota, "post_dashboard_command") as post, mock.patch.object(
            ota, "wait_campaign_status", return_value=status
        ) as wait:
            result = ota.begin_campaign(
                self.args(), "12AB34CD", 168, ["ABC123", "DEF456"], Ledger()
            )
        self.assertEqual(result, status)
        self.assertEqual(
            [call.args[1] for call in post.call_args_list],
            ["uB12AB34CD:168", "uA12AB34CD:ABC123", "uA12AB34CD:DEF456"],
        )
        wait.assert_called_once()
        self.assertEqual(len(emitted), 3)

    def test_freeze_uses_job_scoped_command_and_ack(self):
        class Ledger:
            def emit(self, phase, event, target=None, **kwargs):
                pass

        status = {
            "job_id": "12AB34CD",
            "phase": ota.MAINT_PHASE_FROZEN,
            "active": False,
            "target_count": 2,
        }
        with mock.patch.object(ota, "post_dashboard_command") as post, mock.patch.object(
            ota, "wait_campaign_status", return_value=status
        ):
            ota.freeze_campaign(self.args(), "12AB34CD", 2, Ledger())
        self.assertEqual(post.call_args.args[1], "uF12AB34CD")

    def test_same_revision_needs_observed_post_job_reset(self):
        baseline = {"uptime_ms": 100000, "seq": 900}
        self.assertFalse(
            ota.post_job_reset_seen({"uptime_ms": 110000, "seq": 950}, baseline)
        )
        self.assertTrue(
            ota.post_job_reset_seen({"uptime_ms": 5000, "seq": 12}, baseline)
        )

    def test_fresh_maintenance_preflight_reprobes_identity_and_power(self):
        emitted = []

        class Ledger:
            def emit(self, *args, **kwargs):
                emitted.append((args, kwargs))

        found = {"ABC123": ("192.0.2.4", {"battery_v": 3.4})}
        telemetry = {
            "fixture_id": "ABC123",
            "battery_v": 3.35,
            "supply_v": 0,
            "supply_ma": 0,
            "supply_good": False,
            "maint_status": 1,
        }
        with mock.patch.object(ota, "fetch_json", return_value=telemetry) as fetch:
            ota.maintenance_power_preflight(found, Ledger())
        fetch.assert_called_once_with("http://192.0.2.4/telemetry", 2.0)
        self.assertIs(found["ABC123"][1], telemetry)
        self.assertEqual(emitted[0][0][1], "fresh_maintenance_power")

    def test_fresh_maintenance_preflight_refuses_identity_mismatch(self):
        class Ledger:
            def emit(self, *args, **kwargs):
                pass

        found = {"ABC123": ("192.0.2.4", {})}
        with mock.patch.object(
            ota, "fetch_json", return_value={"fixture_id": "DEF456", "battery_v": 3.4}
        ), self.assertRaisesRegex(SystemExit, "no longer identity-ready"):
            ota.maintenance_power_preflight(found, Ledger())

    def test_job_ledger_exclusive_creates_and_flushes(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "job.jsonl"
            ledger = ota.JobLedger(path, "12AB34CD")
            ledger.emit("PLAN", "started", target_count=2)
            ledger.close()
            self.assertIn('"job_id": "12AB34CD"', path.read_text())
            with self.assertRaises(FileExistsError):
                ota.JobLedger(path, "DEADBEEF")


if __name__ == "__main__":
    unittest.main()
