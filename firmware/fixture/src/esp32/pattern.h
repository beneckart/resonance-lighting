// ADR 0084: synchronized decompression patterns (decompression builds only).
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "../core/packet.h"

void patternInit();                 // after solenoidInit(); loads the stored part
void patternTick();                 // loop context, from decompressionTick()
void patternOnBeat(const NbPatternBeat &beat, uint64_t rxUs);
void patternSerialLine(const char *line); // one complete "!P<id> ..." line
bool patternActive();               // playback session running
uint32_t patternSecondsLeft();
void patternStop(const char *why);
void patternTelemetry(String &json); // appends ,"pattern_*" fields
