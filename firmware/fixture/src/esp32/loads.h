// Central "everything off" for maintenance/sleep/protect transitions.
// P2 extends this with the LED driver's blank+rail-cut sequence; keeping the
// call sites stable now means later phases only touch loads.cpp.
#pragma once

void allLoadsOff(const char *why);
