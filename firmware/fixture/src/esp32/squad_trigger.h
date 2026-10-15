#pragma once
#include <Arduino.h>
#include "espnow_link.h"
#include "../core/fixture_context.h"
#ifndef RES_SQUAD_TRIGGER_TARGET
#define RES_SQUAD_TRIGGER_TARGET 0UL
#endif
bool squadTriggerEnabled();
bool squadTriggerActive();
bool squadTriggerSession(uint16_t minutes);
bool squadTriggerPress(const char *source);
void squadTriggerButton();
void squadTriggerWake();
void squadTriggerTick();
void squadTriggerOnPacket(const RxItem &item);
bool squadTriggerFrame(FrameBuffer &frame, uint8_t pixels);
void squadTriggerTelemetry(String &json);
