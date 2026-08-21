#pragma once

// LilyGO T-Deck Plus (LCD variant) pin map.
// Verified against Xinyuan-LilyGO/T-Deck examples/UnitTest/utilities.h (2026-08-19).
// NOT the T-Deck Pro (e-paper) — its drivers do not transfer (ADR 0037).

// Peripheral power rail. Must be HIGH before ANY peripheral (display, keyboard
// MCU, GPS, mic) responds. Keyboard aux MCU needs ~500 ms after this to boot.
#define TDECK_PIN_POWERON 10

// Shared I2C bus: keyboard aux MCU (ESP32-C3, addr 0x55) + GT911 touch.
#define TDECK_PIN_I2C_SDA 18
#define TDECK_PIN_I2C_SCL 8
#define TDECK_I2C_ADDR_KEYBOARD 0x55
#define TDECK_I2C_ADDR_GT911_A 0x5D  // GT911 strap-dependent
#define TDECK_I2C_ADDR_GT911_B 0x14
#define TDECK_PIN_TOUCH_INT 16
#define TDECK_PIN_KEYBOARD_INT 46

// Shared SPI bus: TFT + SD card + SX1262 (SD and LoRa unused; CS held high).
#define TDECK_PIN_SPI_SCK 40
#define TDECK_PIN_SPI_MOSI 41
#define TDECK_PIN_SPI_MISO 38
#define TDECK_PIN_TFT_CS 12
#define TDECK_PIN_TFT_DC 11
#define TDECK_PIN_TFT_BL 42
#define TDECK_PIN_SDCARD_CS 39
#define TDECK_PIN_RADIO_CS 9

// Trackball: four pulse GPIOs (not quadrature) + center click on the BOOT pin.
// Direction map calibrated on hardware 2026-08-19 (Ben): a=UP b=RIGHT c=DOWN d=LEFT.
#define TDECK_PIN_TB_A 3   // UP
#define TDECK_PIN_TB_B 2   // RIGHT
#define TDECK_PIN_TB_C 15  // DOWN
#define TDECK_PIN_TB_D 1   // LEFT
#define TDECK_PIN_TB_CLICK 0  // BOOT strap pin; INPUT_PULLUP, active low

// Battery ADC behind a 2:1 divider.
#define TDECK_PIN_BAT_ADC 4

// GPS UART (T-Deck Plus). Names from LilyGO utilities.h; probe confirms
// direction and baud at runtime (u-blox M10-class often 38400).
#define TDECK_PIN_GPS_TX 43
#define TDECK_PIN_GPS_RX 44

// ES7210 mic ADC (I2S-style). Capture is M5 scope; M0 only probes I2C ack.
#define TDECK_PIN_ES7210_MCLK 48
#define TDECK_PIN_ES7210_LRCK 21
#define TDECK_PIN_ES7210_SCK 47
#define TDECK_PIN_ES7210_DIN 14

// Speaker I2S (MAX98357A-class DAC).
#define TDECK_PIN_I2S_WS 5
#define TDECK_PIN_I2S_BCK 7
#define TDECK_PIN_I2S_DOUT 6
