# locate -- solver library for RSSI+ToF fixture auto-localization.
#
# Layering rule: this package NEVER imports ops/locate/sim/. Both are imported
# by the CLI scripts (locate_run.py etc). Dependencies: numpy + scipy only;
# matplotlib stays in the CLI layer. See ops/locate/README.md.
