import unittest
from storage_host import ReceiptTracker, campaign_status, exact_targets, observation_age_ms, valid_storage_command


class StorageHostTest(unittest.TestCase):
    def test_explicit_command(self):
        self.assertTrue(valid_storage_command("storage-usb 1234ABCD 9F26C4,F2BE60 CONFIRM-USB-WAKE"))
        self.assertTrue(valid_storage_command("storage-stop 1234ABCD"))
        self.assertTrue(valid_storage_command("storage-status"))
        for command in ["storage-stop 00000000", "storage-usb 1234ABCD 000000 CONFIRM-USB-WAKE",
                        "storage-usb 1234ABCD 9F26C4,9f26c4 CONFIRM-USB-WAKE",
                        "storage-usb 1234ABCD F40344 CONFIRM-USB-WAKE",
                        "storage-usb 1234ABCD 9F26C4 CONFIRM-RESET",
                        "storage-usb 1234ABCD 9F26C4 CONFIRM-USB-WAKE\n"]:
            self.assertFalse(valid_storage_command(command), command)

    def test_capacity_and_separators(self):
        ids = ",".join(f"{i:06X}" for i in range(1, 21))
        self.assertEqual(len(exact_targets(ids)), 20)
        for bad in [ids + ",000015", "9F26C4;F2BE60", "9F26C4,", ""]:
            with self.assertRaises(ValueError):
                exact_targets(bad)

    def test_receipt_needs_matching_request(self):
        t = ReceiptTracker(["9F26C4"], "8EB508")
        receipt = "nb-storage id=9F26C4 source=8EB508 seq=42 mode=2 status=1"
        t.ingest(receipt)
        self.assertFalse(t.complete())
        t.ingest("nb-storage-request target=9F26C4 seq=42 mode=2 seconds=0")
        self.assertTrue(t.complete())
        self.assertFalse(t.statuses["9F26C4"]["electrical_shutdown_verified"])

    def test_failure_survives_late_prepared(self):
        t = ReceiptTracker(["9F26C4"], "8EB508")
        t.ingest("nb-storage-request target=9F26C4 seq=42 mode=2 seconds=0")
        for seq, source in [(41, "8EB508"), (42, "979604")]:
            t.ingest(f"nb-storage id=9F26C4 source={source} seq={seq} mode=2 status=1")
        self.assertFalse(t.complete())
        t.ingest("nb-storage id=9F26C4 source=8EB508 seq=42 mode=2 status=5")
        t.ingest("nb-storage id=9F26C4 source=8EB508 seq=42 mode=2 status=1")
        self.assertEqual(t.statuses["9F26C4"]["storage_status_name"], "entry_failed")

    def test_status_is_exact(self):
        s = campaign_status("nb-storage-campaign job=1234ABCD mode=2 active=1 targets=12 prepared=3 refused=1 remaining_ms=900000 dispatches=40")
        self.assertEqual((s["job"], s["prepared"]), ("1234ABCD", 3))
        self.assertIsNone(campaign_status("nb-storage-campaign job=1234ABCD"))

    def test_cached_observation_keeps_aging(self):
        peer = dict(age_ms=3000, ts_utc="2026-09-13T00:00:00+00:00")
        self.assertEqual(observation_age_ms(peer, "2026-09-13T00:00:08+00:00"), 11000)
        self.assertGreater(observation_age_ms({}, "2026-09-13T00:00:08+00:00"), 1800000)


if __name__ == "__main__":
    unittest.main()
