import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from datetime import datetime, timedelta, timezone

import fleet_storage_usb as runner


class FakeDashboard:
    def __init__(self, wrong_revision=False, late_failure=False, omit_acceptance=False):
        self.clock = 0.0
        self.raw = []
        self.commands = []
        self.job = "00000000"
        self.active = False
        self.started = False
        self.failed = False
        self.wrong_revision = wrong_revision
        self.late_failure = late_failure
        self.omit_acceptance = omit_acceptance

    def iso(self, offset=0):
        return (datetime(2026, 9, 13, tzinfo=timezone.utc) +
                timedelta(seconds=self.clock + offset)).isoformat()

    def emit(self, line):
        self.raw.append(dict(ts_utc=self.iso(len(self.raw) / 1000000), line=line))

    def status(self):
        self.emit(f"nb-storage-campaign job={self.job} mode=2 active={int(self.active)} "
                  f"targets={int(self.started)} prepared={int(self.started and not self.failed)} "
                  f"refused={int(self.failed)} remaining_ms=960000 dispatches=1")

    def urlopen(self, request, timeout=5):
        if isinstance(request, str):
            if self.started and self.late_failure and self.clock >= 3 and not self.failed:
                self.failed = True
                self.emit("nb-storage id=9F26C4 source=8EB508 seq=42 mode=2 status=5")
            data = dict(ts_utc=self.iso(), serial=dict(connected=True),
                        master=dict(id="8EB508", channel=11, firmware_rev="tdeck-0.3.0-storage4"),
                        peers={"9F26C4": dict(firmware_rev="old" if self.wrong_revision else "fx-test",
                            age_ms=int(self.clock * 1000 + 3000), ts_utc=self.iso())},
                        raw=self.raw[-160:])
        else:
            command = json.loads(request.data)["cmd"]
            self.commands.append(command)
            if command == "storage-status":
                self.status()
            elif command.startswith("storage-usb "):
                self.job = command.split()[1]
                self.active = self.started = True
                if not self.omit_acceptance:
                    self.emit(f"nb-storage-host job={self.job} accepted=1 reason=started")
                self.emit("nb-storage-request target=9F26C4 seq=42 mode=2 seconds=0")
                self.emit("nb-storage id=9F26C4 source=8EB508 seq=42 mode=2 status=1")
                self.status()
            elif command == "storage-stop " + self.job:
                self.active = False
                self.status()
            data = dict(ok=True)
        return io.BytesIO(json.dumps(data).encode())

    def sleep(self, seconds):
        self.clock += seconds


class StorageRunnerTest(unittest.TestCase):
    def run_fake(self, fake, dry_run=False):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / "job.jsonl"
            args = ["fleet_storage_usb.py", "--targets", "9F26C4", "--expect-fw", "fx-test",
                    "--job-out", str(out), "--dry-run" if dry_run else "--confirm-usb-wake"]
            with patch("sys.argv", args), patch.object(runner.urllib.request, "urlopen", fake.urlopen), \
                    patch.object(runner.time, "monotonic", lambda: fake.clock), \
                    patch.object(runner.time, "sleep", fake.sleep), patch("sys.stdout", io.StringIO()):
                result = runner.main()
            return result, [json.loads(x) for x in out.read_text().splitlines()]

    def test_dry_run_and_revision_failure_send_nothing(self):
        for fake, dry_run in [(FakeDashboard(), True), (FakeDashboard(wrong_revision=True), False)]:
            result, rows = self.run_fake(fake, dry_run)
            self.assertEqual(result, 0 if dry_run else 2)
            self.assertEqual(fake.commands, [])

    def test_prepared_requires_matching_evidence_and_stop(self):
        fake = FakeDashboard()
        result, rows = self.run_fake(fake)
        self.assertEqual(result, 0)
        final = next(r for r in rows if r["event"] == "job_finished")
        self.assertEqual(final["prepared"], ["9F26C4"])
        self.assertTrue(final["sends_stopped"])
        self.assertFalse(final["electrical_shutdown_verified"])
        self.assertGreaterEqual(fake.clock, 15)

    def test_late_entry_failure_is_not_success(self):
        fake = FakeDashboard(late_failure=True)
        result, rows = self.run_fake(fake)
        self.assertEqual(result, 2)
        final = next(r for r in rows if r["event"] == "job_finished")
        self.assertEqual(final["failed"], {"9F26C4": "entry_failed"})
        self.assertEqual(final["prepared"], [])

    def test_http_queue_ack_does_not_replace_host_acceptance(self):
        fake = FakeDashboard(omit_acceptance=True)
        result, rows = self.run_fake(fake)
        self.assertEqual(result, 2)
        self.assertFalse(fake.active)
        self.assertTrue(any(r["event"] == "job_error" for r in rows))


if __name__ == "__main__":
    unittest.main()
