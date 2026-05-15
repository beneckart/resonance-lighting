# OTA Flash Benchmarks - 2026-05-15

Firmware under test: `firmware/smoke_test`, `smoke-2026-05-15.6`.

Boards:

- C6 + IS31FL3741: `192.168.4.248`
- FeatherS2 Neo: `192.168.4.249`
- Atom Matrix: `192.168.4.250`

Binary sizes:

| Board | Binary bytes |
| --- | ---: |
| C6 + IS31FL3741 | 1,063,888 |
| FeatherS2 Neo | 979,344 |
| Atom Matrix | 994,608 |

## OTA Upload Timing

`ack` is time until HTTP OTA returned `Update complete. Rebooting.`.
`ready` is time until the board served the OTA status page again.

### Strict Sequential

| Board | Ack seconds | Ready seconds |
| --- | ---: | ---: |
| C6 + IS31FL3741 | 7.029 | 13.079 |
| FeatherS2 Neo | 10.080 | 16.133 |
| Atom Matrix | 8.843 | 14.906 |

Total wall time, waiting for each board to be ready before starting the next:
44.123 seconds.

Ack-only sum: 25.952 seconds.

### Parallel Batch

| Board | Ack seconds from batch start | Ready seconds from batch start |
| --- | ---: | ---: |
| C6 + IS31FL3741 | 10.419 | 16.801 |
| FeatherS2 Neo | 12.245 | 18.289 |
| Atom Matrix | 9.321 | 15.395 |

Total wall time for all three to upload and become reachable again:
18.291 seconds.

## USB Upload Timing

USB upload used the already-built binaries, so these timings exclude compile
time.

| Board | USB upload seconds | OTA page ready seconds | esptool app write speed |
| --- | ---: | ---: | ---: |
| C6 + IS31FL3741 | 7.109 | 10.188 | 2102.0 kbit/s |
| FeatherS2 Neo | 13.047 | 16.218 | 1071.1 kbit/s |
| Atom Matrix | 14.287 | 17.515 | 891.0 kbit/s |

FeatherS2 note: the first USB upload attempt from the running app failed with
`OSError: [Errno 71] Protocol error` and left the board in the ESP32-S2 USB
bootloader. A recovery upload from the bootloader succeeded in 22.210 seconds,
and a later normal app-to-app USB upload succeeded in 13.047 seconds.

## Takeaways

- OTA can be parallelized across boards because each fixture hosts its own HTTP
  updater.
- On this three-board bench, parallel OTA reduced verified wall time from
  44.123 seconds to 18.291 seconds.
- USB upload speed varies materially by board. On this bench, the C6 via native
  Espressif USB/JTAG serial was fastest; Atom over FTDI was slowest.
- For 100 fixtures, strict sequential OTA at the measured verified average
  would be roughly 24.5 minutes. Ack-only sequential would be roughly
  14.4 minutes. Parallel OTA is bounded by WiFi/router capacity and retry rate,
  so the practical production workflow should use bounded batches rather than
  assuming all 100 fixtures can update at once on every network.
