#!/usr/bin/env python3
"""Bounded, exact-MAC, read-only USB recovery observation (only t telemetry).

Raw JSONL is authoritative. A locked/replaced status snapshot must never stop
serial capture. Restart with a new prefix and the original absolute deadline;
--prior-log requires a finished record before the new owner may start.
"""
import argparse
from contextlib import contextmanager
import ctypes
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import sys
import tempfile
import time

from fleet_usb_bringup import discover, normalize_mac, open_serial_no_reset


def utc():
    return datetime.now(timezone.utc).isoformat()


def select_port(boards, target):
    matches = [b for b in boards if b.mac == target]
    if len(matches) > 1:
        raise RuntimeError('Multiple USB ports claim the target MAC')
    return matches[0].port if matches else None


def check_prior(path, target):
    last = None
    with path.open(encoding='utf-8') as stream:
        for line in stream:
            if line.strip():
                last = json.loads(line)
    if not last or last.get('kind') != 'finished':
        raise RuntimeError('Previous capture has no finished record')
    if last.get('value', {}).get('target') != target:
        raise RuntimeError('Previous capture has a different target')


@contextmanager
def target_lease(target):
    """OS-released lock, shared by this watcher across output directories."""
    path = Path(tempfile.gettempdir()) / ('resonance-usb-' + target.replace(':', '') + '.lock')
    with path.open('a+b') as stream:
        if stream.seek(0, 2) == 0:
            stream.write(b'0')
            stream.flush()
        stream.seek(0)
        if os.name == 'nt':
            import msvcrt
            msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            yield
        finally:
            stream.seek(0)
            if os.name == 'nt':
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def write_snapshot(path, state, *, replace=os.replace, sleep=time.sleep):
    """Bounded retries; return an error instead of ending the raw capture."""
    temporary = path.with_name(path.name + f'.{os.getpid()}.tmp')
    error = None
    try:
        for attempt in range(3):
            try:
                temporary.write_text(json.dumps(state, indent=2) + '\n', encoding='ascii')
                replace(temporary, path)
                return None
            except OSError as exc:
                error = repr(exc)
                if attempt < 2:
                    sleep((0.02, 0.05)[attempt])
        return error
    finally:
        try:
            temporary.unlink(missing_ok=True)
        except OSError:
            pass


class Evidence:
    def __init__(self, prefix, state):
        self.status = Path(str(prefix) + '.status.json')
        self.state = state
        self.stream = Path(str(prefix) + '.jsonl').open('x', encoding='utf-8', newline='\n')

    def record(self, kind, value):
        self.stream.write(json.dumps({'utc': utc(), 'kind': kind, 'value': value}) + '\n')
        self.stream.flush()

    def checkpoint(self):
        self.state['updated_utc'] = utc()
        error = write_snapshot(self.status, self.state)
        if error:
            self.state['checkpoint_failures'] += 1
            self.record('checkpoint_error', error)
        # Liveness remains visible even when every status-file update fails.
        self.record('health', {k: self.state.get(k) for k in (
            'phase', 'updated_utc', 'samples', 'rounds', 'last_sample_utc',
            'checkpoint_failures') } | {'snapshot_ok': error is None})

    def finish(self):
        # Persist final truth before attempting the optional status snapshot.
        self.state['updated_utc'] = utc()
        self.record('finished', dict(self.state))
        write_snapshot(self.status, self.state)
        self.stream.close()


SAMPLE_KEYS = ('fixture_id', 'fw', 'uptime_ms', 'reset_reason', 'battery_v',
               'battery_ma', 'supply_v', 'supply_ma', 'supply_good',
               'charging_enabled', 'bq_reg16', 'bq_stat0', 'bq_stat1',
               'bq_fault0', 'bq_precharge_ma', 'power_tier', 'guard_stage',
               'guard_interrupted', 'sensor_bits', 'last_sleep_reason', 'last_sleep_s')


def observe(args):
    target = normalize_mac(args.target)
    fixture_id = target.replace(':', '')[-6:]
    end = datetime.fromisoformat(args.deadline_utc.replace('Z', '+00:00'))
    if end.utcoffset() is None:
        raise ValueError('Deadline must include a UTC offset')
    remaining = (end - datetime.now(timezone.utc)).total_seconds()
    if remaining <= 0:
        raise ValueError('Original deadline has already passed')
    if args.prior_log:
        check_prior(args.prior_log, target)
    deadline = time.monotonic() + remaining
    with target_lease(target):
        state = dict(target=target, fixture_id=fixture_id, pid=os.getpid(),
                     phase='running', samples=0, rounds=0, checkpoint_failures=0,
                     started_utc=utc(), deadline_utc=end.isoformat(),
                     evidence=str(Path(str(args.prefix) + '.jsonl').resolve()),
                     release_seen=False, recovered_observed=False, last_sample=None,
                     prior_log=str(args.prior_log) if args.prior_log else None)
        evidence = Evidence(args.prefix, state)
        handle = None
        pending = b''
        last_port = None
        next_query = next_checkpoint = 0
        stop = Path(str(args.prefix) + '.stop')
        sleep_held = False
        try:
            evidence.record('metadata', dict(state) | {
                'operation': 'Only t telemetry; no DTR/RTS reset, hold, guard clear, config or firmware changes'})
            if os.name == 'nt':
                if not ctypes.windll.kernel32.SetThreadExecutionState(0x80000001):
                    raise OSError('Failed to prevent laptop sleep')
                sleep_held = True
            while time.monotonic() < deadline and not stop.exists():
                now = time.monotonic()
                if now >= next_checkpoint:
                    evidence.checkpoint()
                    next_checkpoint = now + 5
                port = select_port(discover(), target)
                if port != last_port:
                    evidence.record('port', port)
                    last_port = port
                if port is None:
                    if handle:
                        handle.close()
                        handle = None
                        evidence.record('closed', state['rounds'])
                    time.sleep(0.25)
                    continue
                if handle is None:
                    try:
                        handle = open_serial_no_reset(port, timeout=0.1, write_timeout=0.5)
                    except OSError as exc:
                        evidence.record('open_error', repr(exc))
                        time.sleep(1)
                        continue
                    state['rounds'] += 1
                    pending = b''
                    next_query = now + 1
                    evidence.record('opened', {'port': port, 'round': state['rounds']})
                try:
                    if now >= next_query:
                        handle.write(b't\n')
                        next_query = now + 2
                    chunk = handle.read(max(1, handle.in_waiting))
                except OSError as exc:
                    evidence.record('transport_error', repr(exc))
                    handle.close()
                    handle = None
                    time.sleep(0.3)
                    continue
                pending += chunk
                if len(pending) > 1024 * 1024:
                    raise RuntimeError('Unterminated serial data exceeded 1 MiB')
                while b'\n' in pending:
                    line, pending = pending.split(b'\n', 1)
                    line = line.decode(errors='replace').strip()
                    if not line:
                        continue
                    evidence.record('serial', {'round': state['rounds'], 'text': line})
                    if 'PROTECT release persisted -> clean reboot' in line:
                        state['release_seen'] = True
                        state['release_utc'] = utc()
                    try:
                        sample = json.loads(line)
                    except ValueError:
                        continue
                    if not isinstance(sample, dict) or 'fixture_id' not in sample:
                        continue
                    if sample['fixture_id'] != fixture_id:
                        raise RuntimeError('Serial telemetry identity does not match USB target')
                    state['samples'] += 1
                    state['last_sample_utc'] = utc()
                    state['last_sample'] = {k: sample.get(k) for k in SAMPLE_KEYS}
                    if sample.get('power_tier') in (0, 1, 2) and sample.get('guard_stage') in (1, 2, 3):
                        state['recovered_observed'] = True  # candidate, never proof by itself
                    evidence.record('sample_summary', state['last_sample'])
            state['phase'] = 'stopped' if stop.exists() else 'deadline_reached'
        except BaseException as exc:
            state['phase'] = 'error'
            state['error'] = repr(exc)
            raise
        finally:
            try:
                if handle:
                    handle.close()
            finally:
                if sleep_held:
                    ctypes.windll.kernel32.SetThreadExecutionState(0x80000000)
                evidence.finish()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target', required=True, help='Exact 12-hex-digit USB MAC')
    parser.add_argument('--prefix', required=True, type=Path)
    parser.add_argument('--deadline-utc', required=True)
    parser.add_argument('--prior-log', type=Path)
    observe(parser.parse_args())


if __name__ == '__main__':
    main()
