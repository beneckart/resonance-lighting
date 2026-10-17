import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import usb_recovery_watch as watch


class RecoveryWatchTests(unittest.TestCase):
    def test_temporary_replace_denial_recovers(self):
        with tempfile.TemporaryDirectory() as directory:
            status = Path(directory) / 'status.json'
            status.write_text('{"old":true}')
            real_replace = watch.os.replace
            attempts = []
            def contested(source, target):
                attempts.append(1)
                if len(attempts) < 3:
                    raise PermissionError('busy')
                real_replace(source, target)
            self.assertIsNone(watch.write_snapshot(status, {'new': True}, replace=contested, sleep=lambda _: None))
            self.assertEqual(len(attempts), 3)
            self.assertEqual(json.loads(status.read_text()), {'new': True})

    def test_permanent_status_failure_keeps_raw_capture_and_finish(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / 'capture'
            state = dict(phase='running', checkpoint_failures=0, samples=0, rounds=0)
            evidence = watch.Evidence(prefix, state)
            with patch.object(watch, 'write_snapshot', return_value='PermissionError busy'):
                evidence.checkpoint()
                evidence.record('serial', {'text': 'still receiving'})
                state['phase'] = 'stopped'
                evidence.finish()
            rows = [json.loads(x) for x in prefix.with_suffix('.jsonl').read_text().splitlines()]
            self.assertEqual(state['checkpoint_failures'], 1)
            self.assertFalse(next(r for r in rows if r['kind']=='health')['value']['snapshot_ok'])
            self.assertEqual(rows[-2]['kind'], 'serial')
            self.assertEqual(rows[-1]['kind'], 'finished')

    def test_failed_replacement_preserves_previous_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            status = Path(directory) / 'status.json'
            status.write_text('old snapshot')
            def denied(*_):
                raise PermissionError('busy')
            self.assertIn('PermissionError', watch.write_snapshot(status, {}, replace=denied, sleep=lambda _: None))
            self.assertEqual(status.read_text(), 'old snapshot')
            self.assertEqual(list(Path(directory).glob('*.tmp')), [])

    def test_existing_raw_evidence_is_never_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory) / 'capture'
            prefix.with_suffix('.jsonl').write_text('original evidence')
            with self.assertRaises(FileExistsError):
                watch.Evidence(prefix, {})
            self.assertEqual(prefix.with_suffix('.jsonl').read_text(), 'original evidence')

    def test_exact_target_and_duplicate_refusal(self):
        target = '68:EE:8F:F3:FD:28'
        other = SimpleNamespace(mac='68:EE:8F:F4:03:48', port='COM8')
        wanted = SimpleNamespace(mac=target, port='COM9')
        self.assertIsNone(watch.select_port([other], target))
        self.assertEqual(watch.select_port([other, wanted], target), 'COM9')
        with self.assertRaises(RuntimeError):
            watch.select_port([wanted, wanted], target)

    def test_prior_owner_must_be_finished_and_same_target(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'prior.jsonl'
            path.write_text(json.dumps({'kind':'health','value':{'target':'x'}}))
            with self.assertRaises(RuntimeError):
                watch.check_prior(path, 'x')
            path.write_text(json.dumps({'kind':'finished','value':{'target':'x'}}))
            watch.check_prior(path, 'x')
            with self.assertRaises(RuntimeError):
                watch.check_prior(path, 'y')


if __name__ == '__main__':
    unittest.main()
