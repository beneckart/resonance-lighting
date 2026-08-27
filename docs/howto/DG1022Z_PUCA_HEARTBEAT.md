# DG1022Z -> PUCA heartbeat input

**Status:** the standalone PUCA hardware/radio baseline passes with HEARTBEAT +
line input as the locked boot default. The performer's actual waveform,
fixture-light fidelity, and ceremony isolation boundary remain unverified.

This path lets a Rigol DG1022Z drive a deep-red heartbeat look on the Resonance
Tree through the existing PUCA bridge. It changes only the optional publisher.
It does not change or flash fixture firmware and it does not create a new mesh
packet.

## Output choice

The DG1022Z has two independent arbitrary-waveform outputs, CH1 and CH2. The
rear `CH1/Sync/Ext Mod/Trig/FSK` and `CH2/Sync/Ext Mod/Trig/FSK` connectors are
multi-purpose timing/control connectors, not third and fourth arbitrary outputs.

For an arbitrary waveform, a rear Sync output is a 50 percent duty-cycle TTL
square wave at the arbitrary waveform's repetition frequency. It can provide
timing to a separately isolated edge-input path, but it does not reproduce the
waveform's internal shape or multiple peaks. Do not feed Sync into the current
AC-coupled, peak-rectified AUDIO input and assume one clean pulse: both square-
wave edges may be detected.

- Use rear Sync only with a bench-proven, separately isolated edge-input path
  when matching repetition timing is sufficient.
- Use a separately isolated analog monitor copy of CH1 or CH2 when the tree
  should follow the actual waveform envelope.
- A passive BNC T is electrically suitable for a normal no-human bench because
  the PUCA faceplate input is high impedance. It is not authorization to bridge
  PUCA into a body-connected system.

Rigol reference: [DG1000Z User's Guide](https://www.rigol.com/dam/global/downloads/brochures/en/user-manual/waveform-generators/DG1000Z_UserGuide_EN.pdf),
section `Sync Output`.

## Human-connection isolation boundary

Do not connect the PUCA, Pod20, laptop, router, scope, or Tree power system to a
branch that is electrically connected to a person's body until the complete
ceremony signal chain has been reviewed by someone qualified for human-connected
electrical equipment.

A BNC T shares signal and ground. Adding PUCA can therefore create a new return
path through the Eurorack supply, USB laptop, inverter, or protective earth even
when the original ceremony chain was intended to float. Do not assume the Rigol
outputs, BNC shells, a generic audio transformer, or an inexpensive ground-loop
isolator provide patient-safe isolation.

The accepted integration must expose a documented, separately isolated monitor
output for PUCA. If that output does not exist, keep the systems electrically
separate and use a dedicated generator/playback channel for PUCA, synchronized
through an appropriately isolated timing path. Verify isolation with the actual
power supplies and cables used in the field.

## No-human bench hookup

Required:

- PUCA DSP Original Edition in the powered Eurorack carrier;
- BNC male -> 3.5 mm mono TS male cable;
- DG1022Z with no electrodes or people connected;
- one named bench fixture, or a photodiode/oscilloscope before involving a
  fixture.

Procedure:

1. Leave the DG output disabled. Connect one front CH output to PUCA faceplate
   AUDIO IN L or R. Do not use a CV or TRIG jack.
2. Set the DG channel load display to `High Z`, DC offset to `0 V`, and start
   with a 1 kHz sine or the heartbeat arbitrary waveform at `1 Vpp`.
3. Boot the PUCA bridge. It should need no laptop and should start in HEARTBEAT,
   line input, controls LOCKED. A normal paw touch only replays the bottom-LED
   status: one long pulse for line, then one short pulse for HEARTBEAT. With
   service USB attached, `t` confirms `mode=HEARTBEAT input=line`.
4. Enable the DG output. With service USB, use `t` to watch `peak`, `wave`, and `clipblocks`.
   `wave` should follow the waveform without waiting for quiet calibration.
5. Raise the DG amplitude only as needed, normally no higher than `4 Vpp` at
   the faceplate. Stop if `clipblocks` increments. The PUCA faceplate attenuates
   the signal about 4:1 before the codec.
6. Top knob controls input sensitivity from 0.25x to 4x. Bottom knob is the
   fleet brightness ceiling. Begin with the brightness ceiling low.
7. Compare the DG waveform and emitted light with a two-channel scope or a
   photodiode. The current path reads 100 ms peak blocks and publishes the
   existing direct-frame contract at about 10 Hz, so it catches narrow pulses
   but cannot reproduce detail faster than the installed fixture latch.
8. Disable the DG output. The tree should go dark while PUCA continues sending
   zero-valued live frames. Then stop PUCA publishing or remove its power and
   verify autonomous behavior returns through the existing three-second
   stale-frame fallback. Input-cable removal alone is not automatic source-loss
   detection and does not currently release the live stream.

## Acceptance before a ceremony

- Exact body-side circuit, isolation method, voltages, currents, and power
  sources are documented by its owner.
- A qualified reviewer approves the isolated monitor boundary with all field
  power and USB connections present.
- The no-human bench shows the intended single/double pulse without clipping.
- Pulling the PUCA input produces a dark live stream; stopping PUCA publishing
  or removing PUCA power returns fixtures to autonomy within about three seconds.
- The PUCA-in-Pod20 range and mixed HEX/RGBW tests from ADR 0035 pass.
