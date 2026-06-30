# COTS / reference-design survey -- 2026-05-08

Status: first-pass web survey. Re-check stock and distributor availability before purchasing.

## Summary

The COTS path is credible enough to treat as a production fallback. The strongest architecture options are:

1. **Preferred LiFePO4 COTS/reference power path:** Adafruit/TI bq25185 charger family, configured for LiFePO4, paired with a high-headroom ESP32 board and a 3x3/5x5 LED board.
2. **Fastest integrated visual prototype:** FeatherS2 Neo, because it already has ESP32-S2, LiPo charging, and a 5x5 RGB LED matrix with user-controlled power.
3. **Simple LiPo fallback power path:** DFRobot DFR0559 Solar Power Manager 5V or FireBeetle solar boards, but only with LiPo/Li-ion batteries.
4. **Best custom-PCBA MCU direction:** WROOM-style pre-certified Espressif module with comfortable headroom, likely ESP32-S3-WROOM or ESP32-C6-WROOM family after sourcing/layout review.

## Charger / power candidates

### TI bq25185

Why it matters:

- 1-cell standalone linear charger.
- Supports Li-ion, Li-polymer, and LiFePO4 chemistries.
- Supports power path and solar/weak-source behavior via VINDPM.
- Up to 1 A charge current.
- Supports up to ~3.125 A discharge/system-load current through the battery FET path.
- Low quiescent current: TI lists 4 uA in battery-only mode.
- Includes battery temperature/fault protections and thermal regulation.

Implication:

- bq25185 should replace CN3058 as the first preferred charger reference.
- Still linear, so thermal testing inside the sealed hat is required.
- The 6-hour safety timer behavior matters; any board tying CE permanently low may require input power cycling or explicit CE control if this affects solar charging.

References:

- https://www.ti.com/product/BQ25185
- https://www.ti.com/tool/BQ25185EVM

### Adafruit bq25185 USB / DC / Solar charger -- product 6091

Important details:

- Uses bq25185.
- USB-C input plus separate DC/solar input.
- Battery and load on JST-PH.
- Default is LiPo/Li-ion 4.2 V; LiFePO4 mode requires cutting/soldering jumpers.
- Load output is regulated to 4.5 V max and can go lower with a low battery.
- Power-path behavior means load can draw from charger input when available, reducing unnecessary battery cycling.
- Adafruit publishes EagleCAD PCB files and schematic/fab print in the Learn guide.
- Product page and Learn guide indicated out-of-stock / low stock depending page crawl; verify before ordering.

Use:

- Best immediate LiFePO4 charger reference.
- Good for bench tests even if not production-stockable.
- If cloned into custom PCBA, copy the proven reference rather than inventing topology.

References:

- https://www.adafruit.com/product/6091
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-lithium-ion-polymer-charger
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-lithium-ion-polymer-charger/downloads

### Adafruit bq25185 with 3.3 V buck board

Important details:

- bq25185 plus a separate 3.3 V buck output rated by Adafruit at 1 A max.
- VIN accepts 5-18 V DC/solar.
- 3.3 V output has an enable pin; low disables output.
- Good candidate for powering ESP32 board or custom logic in a prototype.
- Learn guide says /CE is tied to ground, so the bq25185 safety timer cannot be reset except by removing/reapplying input supply on that specific board.

Use:

- Strong COTS candidate for a LiFePO4 power board if stock is available.
- Also a schematic reference for a custom board with integrated buck.

References:

- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-3-3v-buck-board/overview
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-3-3v-buck-board/pinouts
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-3-3v-buck-board/downloads

### Adafruit bq25185 with 5 V boost board

Important details:

- bq25185 plus TPS61023 5 V boost output.
- Useful for powering downstream boards through 5 V / USB-style input.
- Learn guide warns the boost converter cannot turn on/off high loads cleanly; it may stall over ~200 mA startup load.

Use:

- Useful for testing a USB-powered COTS stack.
- Must be bench-tested with ESP32 + LED startup loads before trusting it.

References:

- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-5v-boost-board/overview
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-5v-boost-board/pinouts
- https://learn.adafruit.com/adafruit-bq25185-usb-dc-solar-charger-with-5v-boost-board/downloads

### DFRobot DFR0559 Solar Power Manager 5V

Important details:

- CN3165-based solar power manager.
- 5 V solar input range 4.4-6 V.
- 5 V fixed MPPT behavior.
- Charges 3.7 V Li battery up to 900 mA from solar/USB.
- Provides 5 V 1 A USB/regulator output.
- Includes battery, solar panel, and output protections.
- Battery chemistry is 3.7 V lithium / LiPo / Li-ion, not LiFePO4.

Use:

- Excellent COTS LiPo fallback power module.
- Not a preferred LiFePO4 path.
- Very easy unskilled assembly: solar input, battery JST/screw option, USB output.

References:

- https://www.dfrobot.com/product-1712.html
- https://wiki.dfrobot.com/sunflower__solar_power_manager_5v_sku__dfr0559

## MCU / integrated-board candidates

### FeatherS2 Neo -- Adafruit product 5629 / Unexpected Maker

Important details:

- ESP32-S2, 240 MHz single-core.
- 4 MB flash, 2 MB PSRAM.
- 2.4 GHz WiFi.
- 3D antenna.
- LiPo battery management.
- 700 mA 3.3 V regulator.
- Integrated 5x5 RGB LED matrix.
- LED matrix has user-controlled power / own LDO; CircuitPython board page says the matrix LDO defaults off, so deep sleep has no matrix current draw.
- Adafruit page showed low stock in the crawl; verify before relying on it.

Use:

- Best immediate optical/firmware COTS prototype.
- Because it is LiPo charging, it is not the preferred LiFePO4 production chemistry unless paired differently.
- Great for validating center-pixel optics, 5x5 modes, standard OTA, ESP-NOW behavior, and hat fit.

References:

- https://www.adafruit.com/product/5629
- https://circuitpython.org/board/unexpectedmaker_feathers2_neo/

### Adafruit 5x5 NeoPixel Grid BFF -- product 5646

Important details:

- 25 addressable 1.5 mm x 1.5 mm SK6805 LEDs.
- Designed to solder to the back of QT Py or XIAO boards, or use pin/socket headers.
- Defaults to pin A3, with jumpers for A1/A2.
- Adafruit product page has quantity pricing and likely open-source design files through Adafruit ecosystem.

Use:

- Good LED layout/reference design.
- Not a no-solder production daughterboard as shipped unless header/socket assembly is solved by a vendor.
- Useful candidate to copy/derive for a custom LED daughterboard.

References:

- https://www.adafruit.com/product/5646

### Adafruit ESP32-S3 Feather -- product 5323 / related variants

Important details:

- ESP32-S3 dual-core 240 MHz.
- Mini module has FCC/CE certification.
- Variants include 8 MB flash no PSRAM, 4 MB flash + 2 MB PSRAM, and w.FL option.
- USB-C and LiPo charging.
- MAX17048 battery monitor on some variants.
- STEMMA QT with switchable power.
- Status NeoPixel has pin-controlled power.
- Deep sleep from LiPo around ~100 uA per Adafruit product text.

Use:

- Strong high-headroom COTS firmware/MCU candidate.
- Pair with separate bq25185 LiFePO4 power board or LiPo fallback power board depending chemistry.
- No integrated 5x5 matrix, so needs LED daughterboard.

References:

- https://www.adafruit.com/product/5323
- https://www.adafruit.com/product/5477
- https://learn.adafruit.com/adafruit-esp32-s3-feather/pinouts

### Unexpected Maker FeatherS3[D]

Important details:

- ESP32-S3 dual-core 240 MHz.
- 16 MB flash, 8 MB PSRAM.
- Two 700 mA 3.3 V regulators.
- LDO2 is user-controlled and auto-shuts down in deep sleep.
- LiPo charging and JST PH connector.
- I2C fuel gauge.
- Onboard 3D antenna plus u.FL connector; onboard selected by default.
- 21 GPIO in Feather format.

Use:

- Very strong COTS MCU candidate when headroom and RF flexibility matter.
- Pair with bq25185 LiFePO4 power reference or LiPo fallback.
- No built-in 5x5 matrix, so needs LED board.

References:

- https://esp32s3.com/feathers3d.html

### DFRobot FireBeetle 2 ESP32-C6 -- product 2771 / SKU DFR1075

Important details:

- ESP32-C6, 160 MHz RISC-V.
- WiFi 6, BLE 5, Zigbee 3.0, Thread 1.3, Matter-capable ecosystem.
- USB-C, 5 V DC, and solar input.
- CN3165 solar management chip.
- Integrated lithium-ion / lithium-polymer battery charging and battery level monitoring.
- ESP32-C6 supports ESP-NOW in Espressif IDF docs.

Use:

- Interesting solar-integrated COTS ESP-NOW prototype.
- Battery chemistry is LiPo/Li-ion; not preferred LiFePO4 path unless charger is bypassed/replaced.
- Good for firmware/radio tests and LiPo fallback.

References:

- https://www.dfrobot.com/product-2771.html
- https://wiki.dfrobot.com/SKU_DFR1075_FireBeetle_2_Board_ESP32_C6
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html

### DFRobot FireBeetle 2 ESP32-C5 -- product 2976 / SKU DFR1222

Important details:

- Product 2976 is ESP32-C5, not ESP32-C6.
- Dual-band 2.4/5 GHz WiFi 6.
- Solar charging input 4.5-6 V.
- Max charging current 0.5 A.
- Sleep current listed as 21 uA.
- 240 MHz RISC-V single-core, 4 MB flash.
- Supports BLE, Zigbee, Thread ecosystem.

Use:

- Potentially useful COTS solar prototype / LiPo fallback.
- Newer C5 ecosystem may be less conservative than S3/C6; verify ESP-NOW, Arduino/ESP-IDF support, and stock before depending on it.
- LiPo/Li-ion path unless proven otherwise.

References:

- https://www.dfrobot.com/product-2976.html

## Custom-PCBA MCU module candidates

### ESP32-S3-WROOM-1 family

Why it matters:

- WROOM-style module with integrated RF complexity hidden inside module.
- Dual-core 240 MHz, up to 16 MB flash and up to 8 MB PSRAM depending variant.
- On-board PCB antenna or external antenna connector variants.
- Good headroom for OTA, logging, animations, and future 2027 modes.

Use:

- Preferred default direction for custom PCBA if sourcing/footprint/JLC assembly works.

References:

- https://www.espressif.com/en/products/modules/esp32-s3-wroom-1u
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

### ESP32-C6-WROOM-1 family

Why it matters:

- WROOM module with PCB antenna or external antenna connector.
- WiFi 6, BLE, Zigbee, Thread.
- 4 MB / 8 MB / 16 MB flash variants in datasheet.
- ESP-NOW support exists in ESP32-C6 IDF docs.

Use:

- Good candidate if future 802.15.4/Zigbee/Thread matters or FireBeetle C6 proves attractive.
- Less compute headroom than S3 but modern radio ecosystem.

References:

- https://documentation.espressif.com/esp32-c6-wroom-1_wroom-1u_datasheet_en.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/network/esp_now.html

### ESP32-C3-WROOM-02 family

Why it matters:

- WROOM-style C3 module with PCB antenna, flash, crystal, RF complexity integrated.
- Simpler than S3/C6, likely lower power, less headroom.

Use:

- Conservative lower-compute option if headroom tests show S3 unnecessary.
- Better aligned with "no custom RF" than the original C3-MINI compactness bias.

References:

- https://www.espressif.com/sites/default/files/documentation/esp32-c3-wroom-02_datasheet_en.pdf

## Fab / programming services

### JLCPCB

- JLCPCB personalized services page says standard SMT orders support partial programming tests and that JLC supports programming after soldering is complete.
- This is not the same as guaranteed pre-programmed ESP32 modules arriving production-ready; contact support with exact part/fixture/firmware needs.
- Plan a pogo/USB flashing jig regardless.

Reference:

- https://jlcpcb.com/help/article/jlcpcb-supported-personalized-services

### PCBWay

- PCBWay says they can program some parts; customer should provide part number and code for checking.
- PCBWay assembly pages say IC programming can be done before soldering and list many supported package types.
- For ESP32 modules, confirm whether they can program the module before assembly, after assembly, or only through a fixture you provide.

References:

- https://www.pcbway.com/helpcenter/pcb_assembly_ordering/Can_you_program_the_components_.html
- https://www.pcbway.com/pcb-assembly.html

## Near-term shopping/test priority

1. bq25185 basic charger board and/or 3.3 V buck variant.
2. FeatherS2 Neo for immediate 5x5 optics/firmware test.
3. High-headroom ESP32-S3 board: Adafruit ESP32-S3 Feather or Unexpected Maker FeatherS3[D].
4. DFRobot Solar Power Manager 5V for LiPo fallback.
5. DFRobot FireBeetle C6/C5 solar board for solar-integrated LiPo fallback testing.
6. Adafruit 5x5 NeoPixel BFF for LED daughterboard layout reference.
7. 1-3 W solar panels, both premium and cheap samples.
8. LiFePO4 18650 cells and LiPo fallback cells, labeled and isolated by chemistry.

## Recommended prototype stacks

### Stack 1 -- preferred LiFePO4 architecture reference

```
5-7 V solar panel
  -> bq25185 LiFePO4-configured charger board
  -> LiFePO4 18650
  -> ESP32-S3 board or custom dev carrier
  -> switchable 5x5 LED daughterboard
```

Purpose: prove preferred chemistry, charger behavior, standard OTA, LED power safety, and RF.

### Stack 2 -- fastest optical prototype

```
LiPo battery / USB power
  -> FeatherS2 Neo
  -> onboard 5x5 LED matrix
```

Purpose: prove center-pixel optics, chromatic fringing, animation modes, firmware structure.

### Stack 3 -- no-solder LiPo production fallback

```
5 V solar panel
  -> DFRobot DFR0559 Solar Power Manager 5V
  -> LiPo battery
  -> short USB cable
  -> ESP32 board / FeatherS2 Neo / FireBeetle
```

Purpose: prove that a very simple unskilled assembly path can keep the project alive if LiFePO4/custom PCBA slips.
