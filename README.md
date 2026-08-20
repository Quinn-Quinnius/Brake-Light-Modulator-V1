# Brake Light Modulator V1

Firmware for an Arduino Uno-compatible ATmega328P board with a 128x32 SSD1306
I2C OLED display.

## Dependencies

The project pins its build platform and libraries in `platformio.ini`:

- Adafruit SSD1306 2.5.17
- Adafruit GFX Library 1.12.6
- Adafruit BusIO 1.17.4
- Arduino AVR core (provides Wire and EEPROM)

PlatformIO downloads these dependencies into ignored local directories, so
generated packages do not need to be committed.

## Build

Install PlatformIO Core and build the firmware:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements-dev.txt
.venv/bin/pio run
```

The compiled upload image is written to `.pio/build/uno/firmware.hex`.

Upload to a connected Arduino Uno with:

```sh
.venv/bin/pio run --target upload
```

If the hardware is a different Arduino-compatible board, change the `board`
value in `platformio.ini` before building or uploading.
