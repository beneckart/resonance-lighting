# 2026-07-10 INA long-run capture (20 Ah cell, CA backyard)

`2026-07-10-ina-20ah-1min.csv` is a 1-minute-mean downsample (195 KB) of a
41-hour, 2.92 M-sample INA219 capture spanning 2026-07-11T00:47Z →
2026-07-12T17:52Z. Columns: `minute_utc, channel, n, bus_v_mean, ma_mean`.
Channels are the three INA addresses on the harness: `0x40`, `0x44`, `0x45`.

The raw log (`2026-07-10-ina-20ah.log`, 274 MB) is **not in git** — it exceeds
GitHub's 100 MB per-file hard limit, and `ops/bench/data/**/*-ina-*.log` is
gitignored. It lives on Ben's workstation; re-derive this CSV with the
minute-bucket downsampler if the raw file is ever re-processed.

At 1-minute means the day/night solar cycle, charge/discharge slopes, and
overnight droop all survive; sub-second transients (strike pulses, MPPT
dither) do not. Use the raw log for anything transient.
