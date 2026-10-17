import json
import unittest
from unittest.mock import patch, MagicMock
from pathlib import Path
import fleet_usb_bringup
from fixture_usb_service import prepare_session


class Port:
    def __init__(self, before=None, ready=True, after=None):
        self.before = dict(fixture_id='F40308', usb_service_ready=False,
                           supply_good=True, ota_pending_verify=False, guard_stage=1)
        if before is not None:
            self.before = before
        self.after = dict(self.before, usb_service_ready=True, usb_service_active=True,
                          load_armed=False, led_rail_on=False, solenoid_gate_on=False)
        self.after.update(after or {})
        self.ready=ready; self.lines=[]; self.writes=[]; self.time=0
    def clock(self):
        self.time += .1
        return self.time
    def write(self, data):
        self.writes.append(data)
        if data == b't\n':
            self.lines.append(json.dumps(self.after if len(self.writes)>1 else self.before).encode())
        elif self.ready:
            self.lines.append(b'usb-service: READY target=F40308 window_ms=30000 stage=1')
    def readline(self):
        return self.lines.pop(0) if self.lines else b''


class Tests(unittest.TestCase):
    def test_prepared(self):
        p=Port()
        self.assertTrue(prepare_session(p,'F40308',clock=p.clock)['usb_service_ready'])
        self.assertEqual(p.writes,[b't\n',b'!BF40308:1\n',b't\n'])
    def test_identity_and_legacy_never_prepare(self):
        for data in ({'fixture_id':'F3FD28'}, {'fixture_id':'F40308'}):
            p=Port(before=data)
            with self.assertRaises(RuntimeError): prepare_session(p,'F40308',clock=p.clock)
            self.assertEqual(p.writes,[b't\n'])
    def test_missing_ack(self):
        p=Port(ready=False)
        with self.assertRaises(RuntimeError): prepare_session(p,'F40308',clock=p.clock)
    def test_fail_closed_readback(self):
        for data in ({'load_armed':True},{'led_rail_on':True},{'solenoid_gate_on':True},
                     {'guard_stage':4},{'usb_service_active':False}):
            p=Port(after=data)
            with self.assertRaises(RuntimeError): prepare_session(p,'F40308',clock=p.clock)
    def test_protect_is_preserved(self):
        p=Port(); p.before['guard_stage']=4; p.after['guard_stage']=4
        self.assertEqual(prepare_session(p,'F40308',clock=p.clock)['guard_stage'],4)
    def test_commission_never_resets_after_refusal(self):
        board=fleet_usb_bringup.UsbBoard('COM8','68:EE:8F:F4:03:08','F40308','','')
        kwargs=dict(esptool='esptool',arduino_cli='arduino',build_path=Path('unused'),
                    expect_fw='test',binary_hash='unused',wifi_check=False,
                    battery_chemistry='lfp',capacity_mah=6000,charge_ma=2000,
                    maintain_v=4.6,allow_battery_present=True,expect_class=None,
                    require_bmp581=False,wifi_semaphore=None)
        with patch('fleet_usb_bringup.open_serial_no_reset',return_value=MagicMock()), \
             patch('fixture_usb_service.prepare_session',side_effect=RuntimeError('refused')), \
             patch('fleet_usb_bringup.run_checked') as run:
            result=fleet_usb_bringup.commission_one(board,**kwargs)
            self.assertIn('refused',result['error']);run.assert_not_called()
        with patch('fleet_usb_bringup.open_serial_no_reset',return_value=MagicMock()), \
             patch('fixture_usb_service.prepare_session',return_value={}), \
             patch('fleet_usb_bringup.run_checked',side_effect=RuntimeError('stop after ROM')) as run:
            fleet_usb_bringup.commission_one(board,**kwargs)
            self.assertEqual(run.call_args.args[0],['esptool','--port','COM8','--after','no-reset','flash-id'])

if __name__=='__main__': unittest.main()
