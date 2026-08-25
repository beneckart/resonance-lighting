#pragma once

// Sparse fixture-side RTC anchor. GPS on the T-Deck is the initial absolute
// source; commissioned DS3231 fixtures provide independent UTC holdover.
void timeAnchorInit(bool haveDs3231);
void timeAnchorTick();

