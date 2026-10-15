#!/usr/bin/env python3
"""Read-only live VL53 hand-range plot; one serial owner, no reset on open.

python ops/bench/marill_presence_live.py
Pause serial for flashing by creating tmp/marill-live.pause; remove to resume.
Logs exclusive-create compact observations in ops/bench/data/Oakland.
"""
import argparse
from collections import deque
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
import time

from fleet_usb_bringup import discover, open_serial_no_reset

ROOT = Path(__file__).resolve().parents[2]
PAGE = Path(__file__).with_name('marill_presence_live.html')
KEYS = ('fw', 'fixture_id', 'uptime_ms', 'battery_v', 'power_tier', 'guard_stage',
        'vl_read_ok', 'vl_reads', 'vl_zone_nearest_mm', 'vl_closest_mm',
        'trigger_enabled', 'trigger_left_s', 'trigger_next_step', 'trigger_count',
        'trigger_ready_mask', 'trigger_presence_armed', 'trigger_last',
        'trigger_tx_errors', 'trigger_refused', 'trigger_peers',
        'protect_retry_enabled', 'protect_retry_phase')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--port', type=int, default=0)
    p.add_argument('--seconds', type=int, default=7200)
    p.add_argument('--pause-file', type=Path, default=ROOT / 'tmp/marill-live.pause')
    p.add_argument('--stop-file', type=Path, default=ROOT / 'tmp/marill-live.stop')
    args = p.parse_args()
    samples = deque(maxlen=300)
    state = {'connection': 'Waiting for Marill', 'sample': None, 'sequence': 0}
    lock = threading.Lock()
    stop = threading.Event()
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    logpath = ROOT / 'ops/bench/data/Oakland' / (stamp + '-F40348-live-presence.jsonl')
    logpath.parent.mkdir(parents=True, exist_ok=True)
    log = logpath.open('x', encoding='utf-8')

    def record(kind, value):
        log.write(json.dumps({'utc': datetime.now(timezone.utc).isoformat(),
                              'kind': kind, 'value': value}) + '\n')
        log.flush()

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def do_GET(self):
            if self.path == '/':
                body, mime = PAGE.read_bytes(), 'text/html; charset=utf-8'
            elif self.path == '/data':
                with lock:
                    data = dict(state, samples=list(samples), now=time.time())
                body, mime = json.dumps(data).encode(), 'application/json'
            else:
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header('Content-Type', mime)
            self.send_header('Cache-Control', 'no-store')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    def reader():
        serial = None
        buf = b''
        next_query = next_discover = 0
        last_frame = None
        last_sensor = None
        sensor_at = time.monotonic()
        try:
            while not stop.is_set():
                if args.pause_file.exists():
                    if serial:
                        serial.close()
                        serial = None
                        record('paused', True)
                    with lock:
                        state['connection'] = 'Paused for USB service'
                    stop.wait(.2)
                    continue
                if serial is None and time.monotonic() >= next_discover:
                    next_discover = time.monotonic() + 1
                    for board in discover():
                        if board.fixture_id != 'F40348':
                            continue
                        try:
                            serial = open_serial_no_reset(board.port, timeout=.03, write_timeout=.5)
                            buf = b''
                            record('open', board.port)
                            print('Marill open on ' + board.port, flush=True)
                        except OSError:
                            pass
                        break
                if serial is None:
                    with lock:
                        state['connection'] = 'Waiting for Marill USB / wake cycle'
                    stop.wait(.1)
                    continue
                try:
                    if time.monotonic() >= next_query:
                        serial.write(b't\n')
                        next_query = time.monotonic() + .2
                    buf += serial.read(serial.in_waiting or 1)
                    while b'\n' in buf:
                        line, buf = buf.split(b'\n', 1)
                        text = line.decode(errors='replace').strip()
                        if not text:
                            continue
                        try:
                            raw = json.loads(text)
                        except ValueError:
                            record('serial', text)
                            continue
                        if raw.get('fixture_id') != 'F40348':
                            raise RuntimeError('Wrong USB identity; stopping this reader')
                        sample = {key: raw.get(key) for key in KEYS}
                        sample['time'] = time.time()
                        sensor = (raw.get('fw'), raw.get('vl_reads'))
                        if sensor != last_sensor:
                            last_sensor, sensor_at = sensor, time.monotonic()
                        sample['sensor_age_s'] = round(time.monotonic() - sensor_at, 3)
                        frame = (raw.get('fw'), raw.get('vl_reads'), raw.get('uptime_ms', 0) // 200)
                        with lock:
                            state['connection'] = 'Connected to Marill'
                            state['sample'] = sample
                            if frame != last_frame:
                                state['sequence'] += 1
                                sample['sequence'] = state['sequence']
                                samples.append(sample)
                        if frame != last_frame:
                            record('sample', sample)
                            last_frame = frame
                    if len(buf) > 65536:
                        buf = b''
                except OSError as exc:
                    record('disconnect', str(exc))
                    serial.close()
                    serial = None
                stop.wait(.01)
        finally:
            if serial:
                serial.close()

    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
    record('metadata', {'target': 'F40348', 'operation': 'read-only USB telemetry',
                        'range_mm': [150, 700], 'rearm_far_mm': 800})
    worker = threading.Thread(target=reader, daemon=True)
    worker.start()
    web = threading.Thread(target=server.serve_forever, daemon=True)
    web.start()
    print('URL http://127.0.0.1:%d/' % server.server_port, flush=True)
    print('LOG ' + str(logpath), flush=True)
    until = time.monotonic() + args.seconds
    try:
        while time.monotonic() < until and worker.is_alive() and not args.stop_file.exists():
            time.sleep(.2)
    finally:
        stop.set()
        worker.join(2)
        server.shutdown()
        server.server_close()
        record('closed', True)
        log.close()


if __name__ == '__main__':
    main()
