import unittest
from storage_events import parse_storage_event


class StorageEventsTest(unittest.TestCase):
    def test_request_keeps_exact_target_sequence_and_mode(self):
        row = parse_storage_event("nb-storage-request target=9F26C4 seq=73 mode=2 seconds=0\n")
        self.assertEqual((row["target"], row["request_seq"], row["storage_mode"]),
                         ("9F26C4", 73, 2))

    def test_prepared_never_claims_electrical_shutdown(self):
        row = parse_storage_event("nb-storage id=9F26C4 source=8EB508 seq=73 mode=2 status=1")
        self.assertEqual(row["storage_status_name"], "prepared")
        self.assertFalse(row["electrical_shutdown_verified"])

    def test_entry_failure_remains_distinct(self):
        row = parse_storage_event("nb-storage id=9F26C4 source=8EB508 seq=73 mode=2 status=5")
        self.assertEqual(row["storage_status_name"], "entry_failed")

    def test_truncated_or_unknown_status_is_not_accepted(self):
        self.assertIsNone(parse_storage_event("nb-storage id=9F26C4 source=8EB508 seq=73"))
        self.assertIsNone(parse_storage_event("nb-storage id=9F26C4 source=8EB508 seq=73 mode=2 status=9"))


if __name__ == "__main__":
    unittest.main()
