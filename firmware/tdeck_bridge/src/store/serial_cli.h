#pragma once

// Line-based provisioning + debug CLI over USB serial (115200).
// This is the ADR 0037 provisioning path: secrets live in NVS, never in code.
// (The single-char nb-* bench command surface arrives with the M1 emitters.)
void serialCliTick();
