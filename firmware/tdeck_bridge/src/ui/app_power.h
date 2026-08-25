#pragma once

// Field rest controls: an expiring electrically-dark lease (radio stays awake)
// or a rails-off/radio-off timer sleep. Every fleet-wide action is confirmed
// on-device with cancel focused by default.
void appPowerOpen();
