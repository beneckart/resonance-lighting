#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "hal_display.h"
#include "pins_tdeck.h"

// Hand-configured LGFX for the T-Deck's ST7789 (240x320 native, rotation 1 ->
// 320x240 landscape, keyboard below the panel). The SPI bus is shared with the
// SD slot and the SX1262; both CS lines are parked high in halDisplayInit().
class LGFX_TDeck : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 mPanel;
  lgfx::Bus_SPI mBus;
  lgfx::Light_PWM mLight;

 public:
  LGFX_TDeck() {
    {
      auto cfg = mBus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = TDECK_PIN_SPI_SCK;
      cfg.pin_mosi = TDECK_PIN_SPI_MOSI;
      cfg.pin_miso = TDECK_PIN_SPI_MISO;
      cfg.pin_dc = TDECK_PIN_TFT_DC;
      mBus.config(cfg);
      mPanel.setBus(&mBus);
    }
    {
      auto cfg = mPanel.config();
      cfg.pin_cs = TDECK_PIN_TFT_CS;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 1;  // land in 320x240 with rotation 0
      cfg.readable = false;     // write-only panel wiring
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.bus_shared = true;
      mPanel.config(cfg);
    }
    {
      auto cfg = mLight.config();
      cfg.pin_bl = TDECK_PIN_TFT_BL;
      cfg.invert = false;
      cfg.freq = 12000;
      cfg.pwm_channel = 7;
      mLight.config(cfg);
      mPanel.setLight(&mLight);
    }
    setPanel(&mPanel);
  }
};

static LGFX_TDeck gDisplay;
static lgfx::LGFX_Sprite gCanvas(&gDisplay);
static bool gCanvasOk = false;

bool halDisplayInit() {
  // Park the other SPI chip-selects before the first display transaction.
  pinMode(TDECK_PIN_SDCARD_CS, OUTPUT);
  digitalWrite(TDECK_PIN_SDCARD_CS, HIGH);
  pinMode(TDECK_PIN_RADIO_CS, OUTPUT);
  digitalWrite(TDECK_PIN_RADIO_CS, HIGH);

  if (!gDisplay.init()) return false;
  gDisplay.setRotation(0);
  gDisplay.setBrightness(0);  // hide init garbage until the first push
  gDisplay.fillScreen(TFT_BLACK);

  // Full-screen 16-bit sprite in PSRAM; single pushSprite per redraw avoids
  // the black flash the cores3 bridge hit with per-frame clears.
  gCanvas.setPsram(true);
  gCanvas.setColorDepth(16);
  gCanvasOk = gCanvas.createSprite(gDisplay.width(), gDisplay.height()) != nullptr;
  return true;
}

void halDisplaySetBacklight(uint8_t v) { gDisplay.setBrightness(v); }

lgfx::LGFX_Device *halDisplayDevice() { return &gDisplay; }

lgfx::LGFX_Sprite *halCanvas() { return gCanvasOk ? &gCanvas : nullptr; }

void halCanvasPush() {
  if (gCanvasOk) gCanvas.pushSprite(0, 0);
}
