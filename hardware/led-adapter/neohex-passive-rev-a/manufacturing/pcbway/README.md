# PCBWay Manufacturing Packet

NeoHEX passive adapter Rev A quick-turn PCBA packet.

Upload these files:

```text
neohex-passive-rev-a-gerbers.zip
bom-pcbway.csv
neohex-passive-rev-a-pos-pcbway.csv
ORDER_NOTES.txt
```

Optional reference files:

```text
drc.rpt
neohex-passive-rev-a-pos-all.csv
```

Recommended PCBWay settings:

```text
PCB quantity: 5 or 10
Assembly quantity: same as PCB quantity
Layers: 2
Board size: 72 mm x 35 mm
Material: FR-4
Thickness: 1.6 mm
Copper: 1 oz
Surface finish: ENIG
Solder mask: green
Assembly side: top only
Assembly type: SMT assembly
Parts source: turnkey or partial turnkey
Through-hole components to assemble: 0
```

If the enquiry asks PCB fabrication counts:

```text
SMT pads total number: 46
Through-holes / drill holes: 14
```

If the enquiry asks assembly counts:

```text
SMD components to place: 6
Through-hole components to place: 0
DNP components: J1, C2
```

Important assembly notes:

- Populate J2, J3, J4, J5, R1, and C1.
- Do not populate J1 for this prototype packet. The J1 pads remain in the
  Gerbers so a connector can be hand-soldered later if needed.
- Do not populate C2 for this prototype packet.
- SJ1, SJ2, SJ3, and SJ4 are solder jumpers in copper, not placed parts. Leave
  all solder jumpers open during assembly.
- TP1-TP5 are bare test pads, not placed parts.
- J5 is the fallback output for an Adafruit 4528-style Grove-to-STEMMA-QT cable:
  `1 GND`, `2 VLED`, `3 NC`, `4 DATA`.
