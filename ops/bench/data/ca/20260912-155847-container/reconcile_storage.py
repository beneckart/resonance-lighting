"""Read-only reconciliation of this authorized run; never sends commands."""
import argparse
import csv
from datetime import datetime, timedelta, timezone
import json
from pathlib import Path
import sys
import urllib.request

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parents[2]))
from storage_host import ReceiptTracker, observation_age_ms

ap = argparse.ArgumentParser()
ap.add_argument('--final', action='store_true')
args = ap.parse_args()
with urllib.request.urlopen('http://127.0.0.1:8765/api/state', timeout=5) as response:
    state = json.load(response)
now = datetime.fromisoformat(state['ts_utc'])
expected = set((ROOT / 'all-106-ota-verified.txt').read_text().split())
installed = set((ROOT / 'expected-20260901.txt').read_text().split())
assert len(expected) == 106 and len(installed) == 114
peers, jobs = [], []
for path in [ROOT / 'storage-canary.jsonl', *sorted(ROOT.glob('storage-[0-9][0-9].jsonl'))]:
    records = []
    for line in path.read_text(encoding='utf-8').splitlines():
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError:
            pass  # an in-progress final line will be read next time
    preflight = next((r for r in records if r['event'] == 'preflight'), None)
    if preflight is None:
        continue
    tracker = ReceiptTracker(preflight['targets'], '8EB508')
    receipt_times = {}
    for record in records:
        if record['event'] != 'bridge_storage_line':
            continue
        matched = tracker.ingest(record['line'])
        if matched and matched['src'] == 'storage_receipt':
            receipt_times.setdefault(matched['peer_id'], record['bridge_ts_utc'])
    final = next((r for r in reversed(records) if r['event'] == 'job_finished'), {})
    jobs.append(dict(ledger=path.name, job_id=preflight['job_id'],
                     targets=preflight['targets'], finished=bool(final),
                     sends_stopped=final.get('sends_stopped', False),
                     errors=[r['error'] for r in records if r['event'] == 'job_error']))
    for target, receipt in tracker.statuses.items():
        peer = state['peers'].get(target, {})
        stamp = datetime.fromisoformat(receipt_times[target])
        last_heard = now - timedelta(milliseconds=observation_age_ms(peer, state['ts_utc']))
        audit = (peer.get('last_command_sleep_cause') == 8 and
                 peer.get('last_command_sleep_s') == 0 and
                 peer.get('last_command_sleep_source') == '8EB508' and
                 peer.get('last_command_sleep_source_seq') == receipt['request_seq'])
        peers.append(dict(id=target, callsign=peer.get('callsign', target),
                          job_id=preflight['job_id'], status=receipt['storage_status_name'],
                          request_source='8EB508', request_seq=receipt['request_seq'],
                          receipt_utc=stamp.isoformat(),
                          last_heard_estimate_utc=last_heard.isoformat(),
                          quiet_elapsed_s=round((now - stamp).total_seconds(), 3),
                          rejoined_after_storage=last_heard > stamp + timedelta(seconds=5),
                          durable_command_audit_matches=audit,
                          battery_v=peer.get('battery_v'),
                          firmware_rev=peer.get('firmware_rev')))

assert len({p['id'] for p in peers}) == len(peers), 'Duplicate target across storage jobs'
prepared = {p['id'] for p in peers if p['status'] == 'prepared'}
earliest = min((datetime.fromisoformat(p['receipt_utc']) for p in peers), default=now)
latest = max((datetime.fromisoformat(p['receipt_utc']) for p in peers), default=now)
master_times, master_uptimes, capture_rows = [], [], 0
for line in (ROOT / 'storage-rollout.jsonl').open(encoding='utf-8'):
    try:
        row = json.loads(line)
    except json.JSONDecodeError:
        continue
    capture_rows += 1
    if row.get('src') == 'master':
        stamp = datetime.fromisoformat(row['ts_utc'])
        if stamp >= earliest - timedelta(seconds=20):
            master_times.append(stamp)
            master_uptimes.append(row.get('uptime_ms'))
gaps = [(b - a).total_seconds() for a, b in zip(master_times, master_times[1:])]
max_gap = max(gaps, default=10**9)
master_fresh = bool(master_times) and (now - master_times[-1]).total_seconds() <= 30
uptime_known = bool(master_uptimes) and all(isinstance(x, int) for x in master_uptimes)
no_bridge_reboot = uptime_known and all(b >= a for a, b in zip(master_uptimes, master_uptimes[1:]))
radio_quiet = all(not p['rejoined_after_storage'] for p in peers)
qualified = (prepared == expected and all(p['durable_command_audit_matches'] for p in peers)
             and all(p['firmware_rev'] == 'fx-260912-f951ae9-b' for p in peers)
             and all(j['finished'] and j['sends_stopped'] and not j['errors'] for j in jobs)
             and radio_quiet and (now - latest).total_seconds() >= 960
             and state['serial']['connected'] and state['master']['id'] == '8EB508'
             and state['master']['firmware_rev'] == 'tdeck-0.3.0-storage4'
             and state['master']['channel'] == 11 and max_gap <= 30
             and master_fresh and no_bridge_reboot)
result = dict(ts_utc=state['ts_utc'], observed_expected=106, installed_expected=114,
              prepared_count=len(prepared), unresolved_observed=sorted(expected-prepared),
              unobserved_installed=sorted(installed-expected), jobs=jobs,
              radio_quiet=radio_quiet, latest_receipt_utc=latest.isoformat(),
              quiet_check_due_utc=(latest+timedelta(seconds=960)).isoformat(),
              quiet_since_last_receipt_s=round((now-latest).total_seconds(), 3),
              capture_rows=capture_rows, max_master_gap_s=max_gap,
              master_fresh=master_fresh, no_bridge_reboot=no_bridge_reboot,
              radio_storage_verification_complete=qualified,
              electrical_current_measured=False, physical_usb_wake_tested=False,
              peers=sorted(peers, key=lambda p: p['id']))
if args.final and not qualified:
    raise SystemExit('Final radio-storage verification is not complete; no final artifact written.')
stem = 'storage-final' if args.final else 'storage-evidence-live'
mode = 'x' if args.final else 'w'
with (ROOT / (stem+'.json')).open(mode, encoding='utf-8', newline='\n') as out:
    json.dump(result, out, indent=2)
if peers:
    with (ROOT / (stem+'.csv')).open(mode, encoding='utf-8', newline='') as out:
        writer = csv.DictWriter(out, fieldnames=list(peers[0]))
        writer.writeheader()
        writer.writerows(result['peers'])
print(json.dumps({k: v for k, v in result.items() if k not in ('peers', 'jobs')}, indent=2))
