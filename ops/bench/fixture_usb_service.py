#!/usr/bin/env python3
"""Prepare one identified, running fixture for deliberate ROM entry.

No resets on refusal, timeout or legacy firmware. Bootstrap old images by OTA
first. READY expires after 30 s; enter ROM immediately, then use --before
no-reset for subsequent ROM operations. This tool never flashes or clears a tier.
"""
import argparse
import json
import re
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path

from fleet_usb_bringup import discover, find_esptool, open_serial_no_reset


def prepare_session(port, target, record=lambda *args: None, clock=time.monotonic):
    # A queued READY from an earlier, expired service window is not proof for
    # a new ROM entry. Discard buffered input before this request/response run.
    port.reset_input_buffer()

    def telemetry():
        port.write(b't\n')
        end = clock() + 5
        while clock() < end:
            line = port.readline().decode(errors='replace').strip()
            if line:
                record('rx', line)
            try:
                data = json.loads(line)
            except ValueError:
                continue
            if isinstance(data, dict) and 'fixture_id' in data:
                if data['fixture_id'] != target:
                    raise RuntimeError('USB identity mismatch; no reset')
                return data
        raise RuntimeError('No fresh telemetry; no reset')

    before = telemetry()
    if 'usb_service_ready' not in before:
        raise RuntimeError('Firmware lacks service handshake; OTA bootstrap required; no reset')
    if before.get('ota_pending_verify') or not before.get('supply_good'):
        raise RuntimeError('Power or pending verification prevents service; no reset')
    command = f'!B{target}:1\n'
    port.write(command.encode('ascii'))
    record('tx', command.strip())
    end = clock() + 5
    expected = f'usb-service: READY target={target} window_ms=30000 stage='
    while clock() < end:
        line = port.readline().decode(errors='replace').strip()
        if line:
            record('rx', line)
        if line.startswith(expected):
            after = telemetry()
            if not (after.get('usb_service_ready') is True and
                    after.get('usb_service_active') is True and
                    after.get('load_armed') is False and
                    after.get('led_rail_on') is False and
                    after.get('solenoid_gate_on') is False and
                    after.get('guard_stage') == before.get('guard_stage')):
                raise RuntimeError('Service readback failed; no reset')
            record('prepared', {'before': before, 'after': after})
            return after
        if line.startswith('usb-service: REFUSED'):
            raise RuntimeError('Fixture refused service; no reset')
    raise RuntimeError('No matching READY acknowledgement; no reset')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--target', required=True)
    p.add_argument('--out', required=True, type=Path)
    p.add_argument('--enter-rom', action='store_true')
    a = p.parse_args()
    target = a.target.upper()
    if not re.fullmatch('[0-9A-F]{6}', target) or target == '000000':
        p.error('one explicit nonzero short MAC required')
    boards = [b for b in discover() if b.fixture_id == target]
    if len(boards) != 1:
        raise RuntimeError('Exact USB target is not present')
    board = boards[0]
    with a.out.open('x', encoding='utf-8') as log:
        def record(kind, value):
            log.write(json.dumps({'utc': datetime.now(timezone.utc).isoformat(),
                                  'kind': kind, 'value': value}) + '\n')
            log.flush()
        with open_serial_no_reset(board.port, timeout=.1, write_timeout=1) as port:
            prepare_session(port, target, record)
        if a.enter_rom:
            args = [find_esptool(), '--chip', 'esp32s3', '--port', board.port,
                    '--baud', '115200', '--no-stub', '--before', 'default-reset',
                    '--after', 'no-reset', 'read-mac']
            r = subprocess.run(args, capture_output=True, text=True, timeout=20)
            record('rom', {'returncode': r.returncode, 'stdout': r.stdout, 'stderr': r.stderr})
            if r.returncode or board.mac.lower() not in r.stdout.lower():
                raise RuntimeError('ROM identity not verified; do not flash')
            print(f'{target}: ROM identity verified; use --before no-reset')
        else:
            print(f'{target}: prepared; enter ROM now or allow bounded clean reboot')


if __name__ == '__main__':
    main()
