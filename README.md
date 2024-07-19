# CubeFX

Inject colorful lighting into your ZimaCube!

CubeFX is a **third-party** open-source lighting control system designed for the ZimaCube, based on the ESP32-C3 and WS2812B LED strips. It allows you to customize lighting effects, colors, and speed, and control them through a web page or buttons.

[![GitHub release](https://img.shields.io/github/v/release/Cp0204/CubeFX.svg)](https://github.com/Cp0204/CubeFX/releases/latest) [![GitHub license](https://img.shields.io/github/license/Cp0204/CubeFX.svg)](https://github.com/Cp0204/CubeFX/blob/main/LICENSE)

[![Demo](https://img.youtube.com/vi/K5UVmzoG0bY/0.jpg)](https://www.youtube.com/watch?v=K5UVmzoG0bY)

## Features

* [x] **Multiple Lighting Effects:** Built-in various lighting effects with support for custom effects
* [x] **Control Interface:** Easy control through a web page
* [x] **AP Toggle:** Ability to turn off the WiFi AP and reopen it with a button or power cycle
* [x] **OTA Update Support:** Update firmware wirelessly without disassembly
* [x] **Multiple Color Formats:** Compatible with RGB, HSV, and HEX color input formats
* [x] **Power-off Memory:** Automatically saves lighting settings to EEPROM, preserving them after power loss
* [ ] ~~**ZimaOS Protocol Compatibility:** Controllable lighting effects using ZimaOS (awaiting official documentation)~~

## Usage

### Method 1: OTA Update

* Download the `CubeFX_ota_xxx.bin` firmware from the [Releases](https://github.com/Cp0204/CubeFX/releases/latest) page
* Connect to the `ZimaCube` AP, the default password is `homecloud`
* Visit http://172.16.1.1 in your browser
* Upload the `.bin` file to the ESP32-C3 development board

### Method 2: Web Flashing

* Open the [CubeFX installer](https://play.cuse.eu.org/cubefx)
* Connect the ESP32-C3 using a Type-C data cable
* Follow the instructions to update the firmware


### Customizing LED Colors

Connect to the AP and open http://172.16.1.1/post

Or use other software to POST data to `http://172.16.1.1/post` in the following format:

```json
{
    "on": 1,
    "id": 5,
    "speed": 128,
    "lightness": 255,
    "data": [
        {
            "h": 0,
            "s": 100,
            "v": 100
        },
        {
            "r": 0,
            "g": 255,
            "b": 0
        },
        "0000FF"
    ]
}
```

* **on:** Light switch, optional, [0,1]
* **id:** Lighting effect, currently valid range is [-71,5]
* **speed:** Effect speed, range [0,255]
* **lightness:** Brightness, range [0,255]
* **data:** Supports RGB, HSV, and HEX color input formats, where HEX is a 6-digit hexadecimal color value. HSV format is planned for deprecation, please use RGB or HEX format.

## Donate

Enjoyed the project? Consider buying me a coffee - it helps me keep going!

<a href="https://buymeacoffee.com/cp0204"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" height="50" width="210" target="_blank"/></a>


## Thank

- [kitesurfer1404/WS2812FX](https://github.com/kitesurfer1404/WS2812FX)
- [IceWhaleTech](https://github.com/IceWhaleTech)