// Serial/HTTP telemetry. telemetryJson() is the fleet_usb_bringup contract:
// one line of JSON in response to serial 't', same body at GET /telemetry.
#pragma once

#include <Arduino.h>

String telemetryJson();
String telemetryBanner(); // GET / body

// Shared observability state, written by the owning modules.
extern uint8_t gTelemetryLifeState;
extern uint8_t gTelemetryPowerTier;
extern uint8_t gTelemetryProgram;
extern uint8_t gTelemetryGuardStage;
extern bool gTelemetryGuardInterrupted;
extern uint8_t gTelemetryFixtureClass;
extern bool gTelemetryClassMismatch;
