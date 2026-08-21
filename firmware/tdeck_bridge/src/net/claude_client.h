#pragma once

#include <stddef.h>
#include <stdint.h>

// Claude Messages-API client: single-flight HTTPS + streaming SSE on its own
// core-0 task (TLS bursts stay off the UI core). Mesh keeps running WAN-down;
// messages queue rather than error (design brief §7).
//
// SNTP-before-TLS is a hard gate: a 1970 clock fails certificate validation
// in a way that looks like a network fault.

enum class ChatState : uint8_t {
  IDLE,       // nothing in flight
  QUEUED,     // message waiting for WAN/SNTP (UI shows "queued", not error)
  CONNECTING, // TLS handshake / request out, no bytes yet ("thinking")
  STREAMING,  // deltas arriving
  BACKOFF,    // transient failure, retrying with capped backoff
  FAILED,     // gave up; claudeStatusLine() carries the reason
};

void claudeBegin();                    // PSRAM buffers + net task (core 0)
bool claudeSubmit(const char *text);   // false = a message is already pending
bool claudeClearHistory();             // false while a message is in flight
ChatState claudeState();
const char *claudeStatusLine();        // short status/error for the UI
uint32_t claudeGeneration();           // bumps when a response begins
// Drain streamed response text (UI task); returns bytes copied.
size_t claudeReadDeltas(char *buf, size_t cap);
void claudeMirrorSerial(bool on);      // echo stream to USB serial (debug)
