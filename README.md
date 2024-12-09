# Firmware for Shades Machine

## Prerequisites

Clone the [Pico SDK](https://github.com/raspberrypi/pico-sdk):

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
cd ..
export PICO_SDK_PATH=<pico-sdk-path>
```

Clone [Picotool](https://github.com/raspberrypi/picotool):

```bash
git clone https://github.com/raspberrypi/picotool.git
sudo apt install build-essential pkg-config libusb-1.0-0-dev cmake
mkdir build
cd build
cmake ..
make
sudo cp -p picotool /usr/local/bin
cd ..
export picotool_DIR=`pwd`
```

Clone the firmware for the shades machine:

```bash
git clone https://github.com/jacehalvorson/pico-shades.git
cd pico-shades
```

## Build the Software

```bash
export PICO_BOARD=pico_w
export WIFI_SSID=<WIFI name>
export WIFI_PASSWORD=<Wi-Fi password>
mkdir build
cd build
cmake ..
make
```

## Flashload the Software

Write the `shades.uf2` file to the drive.

```bash
picotool load -uvx shades.uf2 -f
```

## Monitoring Debug Outputs

Uncommenting the `#define DEBUG` line in `utils.h` allows for serial printout monitoring with `minicom`:

```bash
sudo apt install minicom
minicom --device /dev/ttyACM0
```
