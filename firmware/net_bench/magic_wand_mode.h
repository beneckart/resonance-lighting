#pragma once

// NeoHex Magic Wand LED role for net_bench.
//
// This keeps the proven 20-NeoHex / 740-pixel renderer separate from the fleet
// networking and OTA machinery. The external Pololu supplies the LED rail; this
// module owns only GPIO10 data. Maintenance deliberately blanks the pixels to
// reduce load during a WiFi update, and COMMS automatically restores the default
// animation.

#if NB_MAGIC_WAND

#include "esp32-hal-rmt.h"

namespace MagicWandMode {

constexpr const char *kVersion = "magic-wand-2026-08-19.2";
constexpr uint8_t kDataPin = 10;  // PowerFeather A0 / GPIO10
constexpr uint8_t kBoardCount = 20;
constexpr uint8_t kPixelsPerBoard = 37;
constexpr uint16_t kPixelCount = kBoardCount * kPixelsPerBoard;
constexpr uint16_t kStepMs = 400;
constexpr size_t kBitsPerPixel = 24;
constexpr size_t kSymbolCount = kPixelCount * kBitsPerPixel;

struct Pixel {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

static Pixel pixels[kPixelCount]{};
static rmt_data_t symbols[kSymbolCount]{};
static bool rmtReady = false;
static bool paused = true;
static uint8_t activeRow = 0;
static uint32_t nextFrameMs = 0;
static uint32_t framesShown = 0;

static void clear() {
  for (Pixel &pixel : pixels) pixel = {0, 0, 0};
}

static void encodeByte(uint8_t value, size_t &symbolIndex) {
  for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
    rmt_data_t &symbol = symbols[symbolIndex++];
    symbol.level0 = 1;
    symbol.level1 = 0;
    if (value & mask) {
      symbol.duration0 = 8;  // 0.8 us HIGH
      symbol.duration1 = 4;  // 0.4 us LOW
    } else {
      symbol.duration0 = 4;  // 0.4 us HIGH
      symbol.duration1 = 8;  // 0.8 us LOW
    }
  }
}

static bool show() {
  if (!rmtReady) return false;
  size_t symbolIndex = 0;
  for (const Pixel &pixel : pixels) {
    // M5Stack NeoHex uses WS2812 GRB byte order.
    encodeByte(pixel.green, symbolIndex);
    encodeByte(pixel.red, symbolIndex);
    encodeByte(pixel.blue, symbolIndex);
  }
  const bool ok = rmtWrite(kDataPin, symbols, kSymbolCount, RMT_WAIT_FOR_EVER);
  delayMicroseconds(80);
  if (ok) framesShown++;
  return ok;
}

static void renderUpwardRing() {
  // Exact commissioned low-light palette and geometry: four rows of five Hex
  // boards. The red ring advances from Hex 1-5 through Hex 16-20 every 0.4 s.
  constexpr Pixel kRed = {6, 0, 0};
  constexpr Pixel kRowColors[4] = {
      {4, 2, 0},  // Row 1: orange
      {3, 3, 0},  // Row 2: yellow
      {0, 6, 0},  // Row 3: green
      {0, 0, 6},  // Row 4: blue
  };

  for (uint8_t board = 0; board < kBoardCount; ++board) {
    const uint8_t row = board / 5;
    const Pixel color = row == activeRow ? kRed : kRowColors[row];
    const uint16_t firstPixel = board * kPixelsPerBoard;
    for (uint8_t pixel = 0; pixel < kPixelsPerBoard; ++pixel)
      pixels[firstPixel + pixel] = color;
  }
  show();
  activeRow = (activeRow + 1) % 4;
}

static void begin() {
  pinMode(kDataPin, OUTPUT);
  digitalWrite(kDataPin, LOW);
  clear();
  // Four S3 TX memory blocks are required for a stable 740-pixel frame. The
  // one-block path under-ran during the two-board commissioning test.
  rmtReady = rmtInit(kDataPin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_4, 10000000);
  rmtSetEOT(kDataPin, LOW);
  paused = true;
  activeRow = 0;
  nextFrameMs = 0;
  Serial.printf("magic wand: %s, %u boards, %u pixels, GPIO%u, RMT=%s\n",
                kVersion, (unsigned)kBoardCount, (unsigned)kPixelCount,
                (unsigned)kDataPin, rmtReady ? "ready" : "FAILED");
}

static void pauseForMaintenance() {
  if (!paused) {
    clear();
    show();
  }
  paused = true;
  pinMode(kDataPin, OUTPUT);
  digitalWrite(kDataPin, LOW);
  Serial.println("magic wand: pixels blanked for maintenance");
}

static void resumeComms() {
  paused = false;
  activeRow = 0;
  nextFrameMs = 0;
  Serial.println("magic wand: default 0.4 s upward-ring pattern active");
}

static void tick() {
  if (paused || !rmtReady) return;
  const uint32_t now = millis();
  if ((int32_t)(now - nextFrameMs) < 0) return;
  renderUpwardRing();
  nextFrameMs = now + kStepMs;
}

static void appendTelemetry(String &j) {
  j += ",\"magic_wand\":true";
  j += ",\"magic_wand_fw\":\"" + String(kVersion) + "\"";
  j += ",\"magic_wand_rmt_ready\":";
  j += rmtReady ? "true" : "false";
  j += ",\"magic_wand_paused\":";
  j += paused ? "true" : "false";
  j += ",\"magic_wand_boards\":" + String(kBoardCount);
  j += ",\"magic_wand_pixels\":" + String(kPixelCount);
  j += ",\"magic_wand_next_row\":" + String(activeRow + 1);
  j += ",\"magic_wand_frames\":" + String((unsigned long)framesShown);
}

}  // namespace MagicWandMode

#endif  // NB_MAGIC_WAND
